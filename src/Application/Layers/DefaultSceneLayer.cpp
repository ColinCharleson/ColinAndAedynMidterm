#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Components/Light.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/PlayerController.h"
#include "Gameplay/Components/EnemyScript.h"
#include "Gameplay/Components/TopEnemyScript.h"
#include "Gameplay/Components/Bolt.h"
// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"

 float playerX, playerY;
 float boltX, boltY, boltZ;
 int playerScore;
 bool boltOut;
 bool canShoot;

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		 
		// Basic gbuffer generation with no vertex manipulation
		ShaderProgram::Sptr deferredForward = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		deferredForward->SetDebugName("Deferred - GBuffer Generation");  

		// Our foliage shader which manipulates the vertices of the mesh
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});  
		foliageShader->SetDebugName("Foliage");   

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },  
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing"); 

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our cel shading example
		ShaderProgram::Sptr celShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/cel_shader.glsl" }
		});
		celShader->SetDebugName("Cel Shader");


		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture   = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec      = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex    = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex      = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);

#pragma region Basic Texture Creation
		Texture2DDescription singlePixelDescriptor;
		singlePixelDescriptor.Width = singlePixelDescriptor.Height = 1;
		singlePixelDescriptor.Format = InternalFormat::RGB8;

		float normalMapDefaultData[3] = { 0.5f, 0.5f, 1.0f };
		Texture2D::Sptr normalMapDefault = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		normalMapDefault->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, normalMapDefaultData);

		float solidBlack[3] = { 0.5f, 0.5f, 0.5f };
		Texture2D::Sptr solidBlackTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidBlackTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidBlack);

		float solidGrey[3] = { 0.0f, 0.0f, 0.0f };
		Texture2D::Sptr solidGreyTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidGreyTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidGrey);

		float solidWhite[3] = { 1.0f, 1.0f, 1.0f };
		Texture2D::Sptr solidWhiteTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidWhiteTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidWhite);

#pragma endregion 

		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/ocean/ocean.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" } 
		});
		  
		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();  

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");   

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.AlbedoMap", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
			boxMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.AlbedoMap", monkeyTex);
			monkeyMaterial->Set("u_Material.NormalMap", normalMapDefault);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 50% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(deferredForward); 
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.AlbedoMap", boxTexture); 
			testMaterial->Set("u_Material.Specular", boxSpec);
			testMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// Our foliage vertex shader material 
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.AlbedoMap", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.DiscardThreshold", 0.1f);
			foliageMaterial->Set("u_Material.NormalMap", normalMapDefault);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(celShader);
		{
			toonMaterial->Name = "Toon"; 
			toonMaterial->Set("u_Material.AlbedoMap", boxTexture);
			toonMaterial->Set("u_Material.NormalMap", normalMapDefault);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f); 
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.AlbedoMap", diffuseMap);
			displacementTest->Set("u_Material.NormalMap", normalMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.AlbedoMap", diffuseMap);
			normalmapMat->Set("u_Material.NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand  = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.NormalMapA", normalMapDefault);
			multiTextureMat->Set("u_Material.NormalMapB", normalMapDefault);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f); 
		}

		// Create some lights for our scene
		GameObject::Sptr lightParent = scene->CreateGameObject("Lights");

		for (int ix = 0; ix < 50; ix++) {
			GameObject::Sptr light = scene->CreateGameObject("Light");
			light->SetPostion(glm::vec3(glm::diskRand(25.0f), 1.0f));
			lightParent->AddChild(light);

			Light::Sptr lightComponent = light->Add<Light>();
			lightComponent->SetColor(glm::linearRand(glm::vec3(1.0f), glm::vec3(1.0f)));
			lightComponent->SetRadius(glm::linearRand(200.0f, 200.0f));
			lightComponent->SetIntensity(glm::linearRand(0.1f, 0.4f));
		}

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPostion({ 0, 0, 15 });
			camera->SetRotation({ 0, 0, 0 });

			//camera->Add<SimpleCameraControl>();

			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(20.0f)));
			tiledMesh->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(boxMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}

		GameObject::Sptr player = scene->CreateGameObject("Player");
		{
			// Set position in the scene
			player->SetPostion(glm::vec3(0.0f, -11.0f, 1.0f));
			player->SetRotation(glm::vec3(0.0f, 0.0f, -90.0f));

			// Add some behaviour that relies on the physics body
			player->Add<PlayerController>();

			// Create and attach a renderer for the player
			RenderComponent::Sptr renderer = player->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			renderer->SetMaterial(monkeyMaterial);

			RigidBody::Sptr PlayerRB = player->Add<RigidBody>(RigidBodyType::Dynamic);
			PlayerRB->AddCollider(BoxCollider::Create(glm::vec3(1, 1, 1)));
			PlayerRB->SetLinearDamping(500.0f);

			Light::Sptr lightComponent = player->Add<Light>();
			lightComponent->SetColor(glm::vec3(0.0f,1.0f,0.0f));
			lightComponent->SetRadius(5);
			lightComponent->SetIntensity(1);

		}
		GameObject::Sptr bolt = scene->CreateGameObject("bolt");
		{
			// Set position in the scene
			bolt->SetPostion(glm::vec3(0.0f, -11.0f, 1.0f));
			bolt->SetRotation(glm::vec3(0.0f, 0.0f, -90.0f));
			bolt->SetScale(glm::vec3(0.01f, 0.01f, 0.01f));

			// Add some behaviour that relies on the physics body
			bolt->Add<Bolt>();

			// Create and attach a renderer for the player
			RenderComponent::Sptr renderer = bolt->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			renderer->SetMaterial(monkeyMaterial);

			Light::Sptr lightComponent = bolt->Add<Light>();
			lightComponent->SetColor(glm::vec3(0.0f, 1.0f, 0.0f));
			lightComponent->SetRadius(2);
			lightComponent->SetIntensity(1);
		}

		GameObject::Sptr RightWall = scene->CreateGameObject("RightWall");
		{
			// Set position in the scene
			RightWall->SetPostion(glm::vec3(27.0f, -11.0f, 1.0f));


			RigidBody::Sptr blockRB = RightWall->Add<RigidBody>(RigidBodyType::Static);
			blockRB->AddCollider(BoxCollider::Create(glm::vec3(2, 2, 2)));
		}
		GameObject::Sptr LeftWall = scene->CreateGameObject("LeftWall");
		{
			// Set position in the scene
			LeftWall->SetPostion(glm::vec3(-27.0f, -11.0f, 1.0f));


			RigidBody::Sptr blockRB = LeftWall->Add<RigidBody>(RigidBodyType::Static);
			blockRB->AddCollider(BoxCollider::Create(glm::vec3(2, 2, 2)));
		}
		// Row 1 of enemies
		{
			GameObject::Sptr enemy1 = scene->CreateGameObject("Enemy1");
			{
				// Set position in the scene
				enemy1->SetPostion(glm::vec3(-8.0f, 0.0f, 1.0f));
				enemy1->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy1->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy1->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy1->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy2 = scene->CreateGameObject("enemy2");
			{
				// Set position in the scene
				enemy2->SetPostion(glm::vec3(-4.0f, 0.0f, 1.0f));
				enemy2->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy2->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy2->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy2->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy3 = scene->CreateGameObject("enemy3");
			{
				// Set position in the scene
				enemy3->SetPostion(glm::vec3(0.0f, 0.0f, 1.0f));
				enemy3->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy3->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy3->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy3->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy4 = scene->CreateGameObject("enemy4");
			{
				// Set position in the scene
				enemy4->SetPostion(glm::vec3(4.0f, 0.0f, 1.0f));
				enemy4->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy4->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy4->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy4->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy5 = scene->CreateGameObject("enemy5");
			{
				// Set position in the scene
				enemy5->SetPostion(glm::vec3(8.0f, 0.0f, 1.0f));
				enemy5->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy5->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy5->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy5->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
		}
		// Row 3 of enemies
		{
			GameObject::Sptr enemy13 = scene->CreateGameObject("enemy13");
			{
				// Set position in the scene
				enemy13->SetPostion(glm::vec3(-8.0f, 8.0f, 1.0f));
				enemy13->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy13->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy13->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy13->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy23 = scene->CreateGameObject("enemy23");
			{
				// Set position in the scene
				enemy23->SetPostion(glm::vec3(-4.0f, 8.0f, 1.0f));
				enemy23->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy23->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy23->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy23->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy33 = scene->CreateGameObject("enemy33");
			{
				// Set position in the scene
				enemy33->SetPostion(glm::vec3(0.0f, 8.0f, 1.0f));
				enemy33->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy33->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy33->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy33->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy43 = scene->CreateGameObject("enemy43");
			{
				// Set position in the scene
				enemy43->SetPostion(glm::vec3(4.0f, 8.0f, 1.0f));
				enemy43->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy43->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy43->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy43->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy53 = scene->CreateGameObject("enemy53");
			{
				// Set position in the scene
				enemy53->SetPostion(glm::vec3(8.0f, 8.0f, 1.0f));
				enemy53->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy53->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy53->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy53->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
		}
		// Row 2 of enemies
		{
			GameObject::Sptr enemy12 = scene->CreateGameObject("enemy12");
			{
				// Set position in the scene
				enemy12->SetPostion(glm::vec3(-8.0f, 4.0f, 1.0f));
				enemy12->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy12->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy12->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy12->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy22 = scene->CreateGameObject("enemy22");
			{
				// Set position in the scene
				enemy22->SetPostion(glm::vec3(-4.0f, 4.0f, 1.0f));
				enemy22->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy22->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy22->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy22->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy32 = scene->CreateGameObject("enemy32");
			{
				// Set position in the scene
				enemy32->SetPostion(glm::vec3(0.0f, 4.0f, 1.0f));
				enemy32->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy32->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy32->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy32->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy42 = scene->CreateGameObject("enemy42");
			{
				// Set position in the scene
				enemy42->SetPostion(glm::vec3(4.0f, 4.0f, 1.0f));
				enemy42->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy42->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy42->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy42->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
			GameObject::Sptr enemy52 = scene->CreateGameObject("enemy52");
			{
				// Set position in the scene
				enemy52->SetPostion(glm::vec3(8.0f, 4.0f, 1.0f));
				enemy52->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

				// Add some behaviour that relies on the physics body
				enemy52->Add<EnemyScript>();

				// Create and attach a renderer for the player
				RenderComponent::Sptr renderer = enemy52->Add<RenderComponent>();
				renderer->SetMesh(monkeyMesh);
				renderer->SetMaterial(monkeyMaterial);

				Light::Sptr lightComponent = enemy52->Add<Light>();
				lightComponent->SetColor(glm::vec3(1.0f, 0.0f, 0.0f));
				lightComponent->SetRadius(5);
				lightComponent->SetIntensity(1);
			}
		}
		GameObject::Sptr topShip = scene->CreateGameObject("topShip");
		{
			// Set position in the scene
			topShip->SetPostion(glm::vec3(9.0f, 12.0f, 1.0f));
			topShip->SetRotation(glm::vec3(0.0f, 0.0f, 90.0f));

			// Add some behaviour that relies on the physics body
			topShip->Add<TopEnemyScript>();

			// Create and attach a renderer for the player
			RenderComponent::Sptr renderer = topShip->Add<RenderComponent>();
			renderer->SetMesh(monkeyMesh);
			renderer->SetMaterial(monkeyMaterial);

			Light::Sptr lightComponent = topShip->Add<Light>();
			lightComponent->SetColor(glm::vec3(1.0f, 0.8f, 0.0f));
			lightComponent->SetRadius(5);
			lightComponent->SetIntensity(2);
		}
		/////////////////////////// UI //////////////////////////////
		/*
		GameObject::Sptr canvas = scene->CreateGameObject("UI Canvas");
		{
			RectTransform::Sptr transform = canvas->Add<RectTransform>();
			transform->SetMin({ 16, 16 });
			transform->SetMax({ 256, 256 });

			GuiPanel::Sptr canPanel = canvas->Add<GuiPanel>();


			GameObject::Sptr subPanel = scene->CreateGameObject("Sub Item");
			{
				RectTransform::Sptr transform = subPanel->Add<RectTransform>();
				transform->SetMin({ 10, 10 });
				transform->SetMax({ 128, 128 });

				GuiPanel::Sptr panel = subPanel->Add<GuiPanel>();
				panel->SetColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

				panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/upArrow.png"));

				Font::Sptr font = ResourceManager::CreateAsset<Font>("fonts/Roboto-Medium.ttf", 16.0f);
				font->Bake();

				GuiText::Sptr text = subPanel->Add<GuiText>();
				text->SetText("Hello world!");
				text->SetFont(font);

				monkey1->Get<JumpBehaviour>()->Panel = text;
			}

			canvas->AddChild(subPanel);
		}
		*/

		/*GameObject::Sptr particles = scene->CreateGameObject("Particles");
		{
			ParticleSystem::Sptr particleManager = particles->Add<ParticleSystem>();  
			particleManager->AddEmitter(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 10.0f), 10.0f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)); 
		}*/

		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);
	}
}

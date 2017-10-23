#include "storiesview.h"
#include <Xinput.h>

const char *levelNames[] = {
#ifdef LCS
	"INDUST",
	"COMMER",
	"SUBURB",
	"UNDERG",
#else
	"BEACH",
	"MAINLA",
	"MALL",
#endif
	nil
};

int32 atmOffset;	// UNUSED, just for rslconv.cpp

rw::V3d zero = { 0.0f, 0.0f, 0.0f };
SceneGlobals Scene;
rw::EngineStartParams engineStartParams;
rw::Material *cubeMat;
rw::Geometry *cubeGeo;
rw::Light *pAmbient;
int drawCubes = 0;
int drawLOD = 1;
int frameCounter = -1;

CTimeCycle *pTimecycle;
rw::RGBA currentAmbient;
rw::RGBA currentEmissive;
rw::RGBA currentSkyTop;
rw::RGBA currentSkyBot;
rw::RGBA currentFog;


int curSectX = 28;
int curSectY = 4;
int curIntr = -1;
int curHour = 12;
int curWeather = 0;

void
panic(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

void
debug(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	fflush(stdout);
	va_end(ap);
}

#define XINPUT
#ifdef XINPUT
int pads[4];
int numPads;
int currentPad;

void
plAttachInput(void)
{
	int i;
	XINPUT_STATE state;

	for(i = 0; i < 4; i++)
		if(XInputGetState(i, &state) == ERROR_SUCCESS)
			pads[numPads++] = i;
}

void
plCapturePad(int arg)
{
	currentPad = arg;
	return;
}

void
plUpdatePad(CControllerState *state)
{
	XINPUT_STATE xstate;
	int pad;

	pad = currentPad < numPads ? pads[currentPad] : -1;
	if(pad < 0 || XInputGetState(pad, &xstate) != ERROR_SUCCESS){
		memset(state, 0, sizeof(CControllerState));
		return;
	}

	state->leftX  = 0;
	state->leftY  = 0;
	state->rightX = 0;
	state->rightY = 0;
	if(xstate.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE || xstate.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		state->leftX = xstate.Gamepad.sThumbLX;
	if(xstate.Gamepad.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE || xstate.Gamepad.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		state->leftY = xstate.Gamepad.sThumbLY;
	if(xstate.Gamepad.sThumbRX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE || xstate.Gamepad.sThumbRX < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
		state->rightX = xstate.Gamepad.sThumbRX;
	if(xstate.Gamepad.sThumbRY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE || xstate.Gamepad.sThumbRY < -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
		state->rightY = xstate.Gamepad.sThumbRY;

	state->triangle = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_Y);
	state->circle = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_B);
	state->cross = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_A);
	state->square = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_X);
	state->l1 = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
	state->l2 = xstate.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
	state->leftshock = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
	state->r1 = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
	state->r2 = xstate.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
	state->rightshock = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
	state->select = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_BACK);
	state->start = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_START);
	state->up = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP);
	state->right = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
	state->down = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
	state->left = !!(xstate.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT);

}
#endif

void
Init(void)
{
	sk::globals.windowtitle = "Stories viewer";
	sk::globals.width = 640;
	sk::globals.height = 480;
	sk::globals.quit = 0;
}

bool
attachPlugins(void)
{
	rw::ps2::registerPDSPlugin(40);
	rw::ps2::registerPluginPDSPipes();

	rw::registerMeshPlugin();
	rw::registerNativeDataPlugin();
	rw::registerAtomicRightsPlugin();
	rw::registerMaterialRightsPlugin();
	rw::xbox::registerVertexFormatPlugin();
	rw::registerSkinPlugin();
	rw::registerUserDataPlugin();
	rw::registerHAnimPlugin();
	rw::registerMatFXPlugin();
	rw::registerUVAnimPlugin();
	rw::ps2::registerADCPlugin();
	return true;
}

void
updateTimecycle(void)
{
	if(curWeather >= 0){
		currentAmbient.red = pTimecycle->m_nAmbientRed[curHour][curWeather];
		currentAmbient.green = pTimecycle->m_nAmbientGreen[curHour][curWeather];
		currentAmbient.blue = pTimecycle->m_nAmbientBlue[curHour][curWeather];
		currentEmissive.red = pTimecycle->m_nAmbientRed_Bl[curHour][curWeather];
		currentEmissive.green = pTimecycle->m_nAmbientGreen_Bl[curHour][curWeather];
		currentEmissive.blue = pTimecycle->m_nAmbientBlue_Bl[curHour][curWeather];
		currentSkyTop.red = pTimecycle->m_nSkyTopRed[curHour][curWeather];
		currentSkyTop.green = pTimecycle->m_nSkyTopGreen[curHour][curWeather];
		currentSkyTop.blue = pTimecycle->m_nSkyTopBlue[curHour][curWeather];
		currentSkyTop.alpha = 255;
		currentSkyBot.red = pTimecycle->m_nSkyBottomRed[curHour][curWeather];
		currentSkyBot.green = pTimecycle->m_nSkyBottomGreen[curHour][curWeather];
		currentSkyBot.blue = pTimecycle->m_nSkyBottomBlue[curHour][curWeather];
		currentSkyBot.alpha = 255;

		pTimecycle->m_fCurrentWaterRed = pTimecycle->m_fWaterRed[curHour][curWeather];
		pTimecycle->m_fCurrentWaterGreen = pTimecycle->m_fWaterGreen[curHour][curWeather];
		pTimecycle->m_fCurrentWaterBlue = pTimecycle->m_fWaterBlue[curHour][curWeather];
		pTimecycle->m_fCurrentWaterAlpha = pTimecycle->m_fWaterAlpha[curHour][curWeather];
		if(pTimecycle->m_fCurrentWaterAlpha < 200)	// not 100% sure this is right
			pTimecycle->m_fCurrentWaterAlpha = 200;
		TheCamera.m_rwcam->setFarPlane(pTimecycle->m_fFarClip[curHour][curWeather]);
		TheCamera.m_rwcam->fogPlane = pTimecycle->m_fFogStart[curHour][curWeather];
	}else{
		currentAmbient.red = 255;
		currentAmbient.green = 255;
		currentAmbient.blue = 255;

#ifdef LCS
		currentEmissive.red = 100;
		currentEmissive.green = 100;
		currentEmissive.blue = 100;
#else
		currentEmissive.red = 25;
		currentEmissive.green = 25;
		currentEmissive.blue = 25;
#endif

		currentSkyTop.red = 128;
		currentSkyTop.green = 128;
		currentSkyTop.blue = 128;
		currentSkyTop.alpha = 255;
		currentSkyBot.red = 128;
		currentSkyBot.green = 128;
		currentSkyBot.blue = 128;
		currentSkyBot.alpha = 255;
		pTimecycle->m_fCurrentWaterRed = 255;
		pTimecycle->m_fCurrentWaterGreen = 255;
		pTimecycle->m_fCurrentWaterBlue = 255;
		pTimecycle->m_fCurrentWaterAlpha = 255;
		TheCamera.m_rwcam->setFarPlane(5000.0f);
		TheCamera.m_rwcam->fogPlane = 5000.0f;
	}
	currentFog.red = (currentSkyTop.red + 2*currentSkyBot.red)/3.0f;
	currentFog.green = (currentSkyTop.green + 2*currentSkyBot.green)/3.0f;
	currentFog.blue = (currentSkyTop.blue + 2*currentSkyBot.blue)/3.0f;
	currentFog.alpha = 255;

}

void
DefinedState(void)
{
	SetRenderState(rw::ZTESTENABLE, 1);
	SetRenderState(rw::ZWRITEENABLE, 1);
	SetRenderState(rw::VERTEXALPHA, 0);
	SetRenderState(rw::SRCBLEND, rw::BLENDSRCALPHA);
	SetRenderState(rw::DESTBLEND, rw::BLENDINVSRCALPHA);
	SetRenderState(rw::FOGENABLE, 0);
	SetRenderState(rw::ALPHATESTREF, 10);
	SetRenderState(rw::ALPHATESTFUNC, rw::ALPHAGREATEREQUAL);
	SetRenderState(rw::FOGCOLOR, *(uint32*)&currentFog);
}

void
makeCube(void)
{
	using namespace rw;

	cubeMat = Material::create();

	cubeGeo = Geometry::create(8, 12, Geometry::MODULATE | Geometry::LIGHT);
	cubeGeo->matList.appendMaterial(cubeMat);

	MorphTarget *mt = &cubeGeo->morphTargets[0];
	V3d *verts = mt->vertices;
	Triangle *tri = cubeGeo->triangles;

	float size = 1.0f;

	verts[0].set(-size, -size,  size);
	verts[1].set(-size,  size,  size);
	verts[2].set( size, -size,  size);
	verts[3].set( size,  size,  size);
	verts[4].set(-size, -size, -size);
	verts[5].set(-size,  size, -size);
	verts[6].set( size, -size, -size);
	verts[7].set( size,  size, -size);

	// top
	tri[0].v[0] = 0; tri[0].v[1] = 1; tri[0].v[2] = 2; tri[0].matId = 0;
	tri[1].v[0] = 2; tri[1].v[1] = 1; tri[1].v[2] = 3; tri[1].matId = 0;

	// bottom
	tri[2].v[0] = 4; tri[2].v[1] = 5; tri[2].v[2] = 6; tri[2].matId = 0;
	tri[3].v[0] = 5; tri[3].v[1] = 6; tri[3].v[2] = 7; tri[3].matId = 0;

	// front
	tri[4].v[0] = 0; tri[4].v[1] = 4; tri[4].v[2] = 6; tri[4].matId = 0;
	tri[5].v[0] = 0; tri[5].v[1] = 6; tri[5].v[2] = 2; tri[5].matId = 0;

	// back
	tri[6].v[0] = 1; tri[6].v[1] = 5; tri[6].v[2] = 7; tri[6].matId = 0;
	tri[7].v[0] = 1; tri[7].v[1] = 7; tri[7].v[2] = 3; tri[7].matId = 0;

	// left
	tri[8].v[0] = 1; tri[8].v[1] = 5; tri[8].v[2] = 4; tri[8].matId = 0;
	tri[9].v[0] = 1; tri[9].v[1] = 4; tri[9].v[2] = 0; tri[9].matId = 0;

	// right
	tri[10].v[0] = 3; tri[10].v[1] = 7; tri[10].v[2] = 6; tri[10].matId = 0;
	tri[11].v[0] = 3; tri[11].v[1] = 6; tri[11].v[2] = 2; tri[11].matId = 0;

	cubeGeo->buildMeshes();
	cubeGeo->calculateBoundingSphere();
};

bool
InitRW(void)
{
//	rw::platform = rw::PLATFORM_D3D8;
	if(!sk::InitRW())
		return false;
	Scene.world = rw::World::create();

	pAmbient = rw::Light::create(rw::Light::AMBIENT);
//	pAmbient->setColor(0.2f, 0.2f, 0.2f);
	pAmbient->setColor(1.0f, 1.0f, 1.0f);
	Scene.world->addLight(pAmbient);

	rw::V3d xaxis = { 1.0f, 0.0f, 0.0f };
	rw::Light *direct = rw::Light::create(rw::Light::DIRECTIONAL);
	direct->setColor(0.8f, 0.8f, 0.8f);
	direct->setFrame(rw::Frame::create());
	direct->getFrame()->rotate(&xaxis, 180.0f, rw::COMBINEREPLACE);
	Scene.world->addLight(direct);

	Scene.camera = sk::CameraCreate(sk::globals.width, sk::globals.height, 1);
	Scene.camera->setFarPlane(5000.0f);
	Scene.camera->setNearPlane(0.9f);
	TheCamera.m_rwcam = Scene.camera;
	TheCamera.m_aspectRatio = 640.0f/480.0f;

#ifdef LCS
	TheCamera.m_position.set(1356.0f, -1107.0f, 96.0f);
	TheCamera.m_target.set(1276.0f, -984.0f, 68.0f);
#endif
#ifdef VCS
//	TheCamera.m_position.set(292.0f, -1402.0f, 71.0f);
	TheCamera.m_position.set(292.0f, -1402.0f, 0.0f);
//	TheCamera.m_target.set(223.0f, -1268.0f, 41.0f);
	TheCamera.m_target.set(223.0f, -1268.0f, 0.0f);
#endif


	Scene.world->addCamera(Scene.camera);

	makeCube();

	Renderer::buildingPipe = makeBuildingPipe();

	return true;
}

bool
GetIsTimeInRange(uint8 h1, uint8 h2)
{
	if(h1 > h2)
		return curHour >= h1 || curHour < h2;
	else
		return curHour >= h1 && curHour < h2;
}

RslTexture*
dumpTexNames(RslTexture *texture, void *pData)
{
	printf("%s\n", texture->name);
	return texture;
}

void
InitGame(void)
{
	Zfile *zfile;
	sk::args.argv++;
	sk::args.argc--;
	if(sk::args.argc > 0){
		SetCurrentDirectory(*sk::args.argv);
		sk::args.argv++;
		sk::args.argc--;
	}
	eLevel levelToLoad = (eLevel)1;
	if(sk::args.argc > 0){
		const char *levelname = *sk::args.argv;
		int i;
		const char **names = levelNames;
		for(i = 0; *names; names++, i++){
			if(strcmpi(*names, levelname) == 0){
				levelToLoad = (eLevel)(i+1);
				goto found;
			}
		}
		panic("unknown level");
found:
		sk::args.argv++;
		sk::args.argc--;
	}

#ifdef LCS
	zfile = zopen("CHK/PS2/GAME.DTZ", "rb");
#else
	zfile = zopen("PS2/GAME.DTZ", "rb");
#endif
	if(zfile == nil){
		sk::globals.quit = 1;
		return;
	}

	sChunkHeader header;
	zread(zfile, &header, sizeof(sChunkHeader));
	uint8 *data = (uint8*)malloc(header.fileSize-sizeof(sChunkHeader));
	zread(zfile, data, header.fileSize-sizeof(sChunkHeader));
	zclose(zfile);
	cReloctableChunk(header.ident, header.shrink).Fixup(header, data);

	ResourceImage *resimg = (ResourceImage*)data;
	pBuildingPool = (BuildingPool*)resimg->buildingPool;
	pTreadablePool = (TreadablePool*)resimg->treadablePool;
	pDummyPool = (DummyPool*)resimg->dummyPool;
	CTexListStore::Initialize((TexListPool*)resimg->texlistPool);
	CModelInfo::Load(resimg->numModelInfos, resimg->modelInfoPtrs);
	pTimecycle = resimg->timecycle;
	CWaterLevel_::Initialize(resimg->waterLevelInst);

	LoadLevel(levelToLoad);
	int i;
	for(i = 0; i < gLevel->numWorldSectors; i++)
		LoadSector(i);
	for(i = 0; i < gLevel->chunk->numInteriors; i++)
		LoadSector(gLevel->chunk->interiors[i].sectorId);

#ifdef VCS
	for(i = 0; i < gLevel->chunk->numAreas; i++)
		LoadArea(i);
#endif
}

// Arguments:
// 0---1
// |   |
// 2---3
rw::RWDEVICE::Im2DVertex quadverts[4];
static short quadindices[] = {
	0, 1, 2,
	0, 2, 3
};
void
setQuadVertices(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3,
	rw::RGBA c0, rw::RGBA c1, rw::RGBA c2, rw::RGBA c3)
{
	// This is what we draw:
	// 3---2
	// | / |
	// 0---1
	quadverts[0].setScreenX(x2);
	quadverts[0].setScreenY(y2);
	quadverts[0].setScreenZ(rw::im2d::GetNearZ());
	quadverts[0].setCameraZ(Scene.camera->nearPlane);
	quadverts[0].setRecipCameraZ(1.0f/Scene.camera->nearPlane);
	quadverts[0].setColor(c2.red, c2.green, c2.blue, c2.alpha);
	quadverts[0].setU(0.0f);
	quadverts[0].setV(0.0f);

	quadverts[1].setScreenX(x3);
	quadverts[1].setScreenY(y3);
	quadverts[1].setScreenZ(rw::im2d::GetNearZ());
	quadverts[1].setCameraZ(Scene.camera->nearPlane);
	quadverts[1].setRecipCameraZ(1.0f/Scene.camera->nearPlane);
	quadverts[1].setColor(c3.red, c3.green, c3.blue, c3.alpha);
	quadverts[1].setU(1.0f);
	quadverts[1].setV(0.0f);

	quadverts[2].setScreenX(x1);
	quadverts[2].setScreenY(y1);
	quadverts[2].setScreenZ(rw::im2d::GetNearZ());
	quadverts[2].setCameraZ(Scene.camera->nearPlane);
	quadverts[2].setRecipCameraZ(1.0f/Scene.camera->nearPlane);
	quadverts[2].setColor(c1.red, c1.green, c1.blue, c1.alpha);
	quadverts[2].setU(1.0f);
	quadverts[2].setV(1.0f);

	quadverts[3].setScreenX(x0);
	quadverts[3].setScreenY(y0);
	quadverts[3].setScreenZ(rw::im2d::GetNearZ());
	quadverts[3].setCameraZ(Scene.camera->nearPlane);
	quadverts[3].setRecipCameraZ(1.0f/Scene.camera->nearPlane);
	quadverts[3].setColor(c0.red, c0.green, c0.blue, c0.alpha);
	quadverts[3].setU(0.0f);
	quadverts[3].setV(1.0f);
}
void
renderQuad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3,
	rw::RGBA c0, rw::RGBA c1, rw::RGBA c2, rw::RGBA c3)
{
	rw::SetRenderState(rw::VERTEXALPHA, c0.alpha != 255 || c1.alpha != 255 || c2.alpha != 255 || c3.alpha != 255);
	rw::SetRenderState(rw::ZTESTENABLE, 0);
	rw::SetRenderState(rw::ZWRITEENABLE, 0);
	rw::engine->imtexture = nil;
	setQuadVertices(x0, y0, x1, y1, x2, y2, x3, y3, c0, c1, c2, c3);
	rw::im2d::RenderIndexedPrimitive(rw::PRIMTYPETRILIST,
		&quadverts, 4, &quadindices, 6);
	rw::SetRenderState(rw::ZTESTENABLE, 1);
	rw::SetRenderState(rw::ZWRITEENABLE, 1);
}

float
calcHorizonCoords(void)
{
	rw::Matrix *mat = TheCamera.m_rwcam->getFrame()->getLTM();
	rw::Matrix view;

	rw::V3d pos = mat->pos;
	pos.z = 0.0f;
	pos.x += 3000.0f * mat->at.x;
	pos.y += 3000.0f * mat->at.y;
	rw::Matrix::invert(&view, mat);
	rw::V3d::transformPoints(&pos, &pos, 1, &TheCamera.m_rwcam->viewMatrix);
	return pos.y * sk::globals.height / pos.z;
}

float horizonz;
rw::RGBA skyTop;
rw::RGBA skyBot;
rw::RGBA bgcolor;

#define SMALLSTRIPHEIGHT 4.0f
#define HORIZSTRIPHEIGHT 32.0f

void
drawBackground(void)
{
	rw::Matrix *mat = TheCamera.m_rwcam->getFrame()->getLTM();
//	float l = sqrt(mat->right.x * mat->right.x +
//		mat->right.y * mat->right.y);
//	if(l > 1.0f)
//		l = 1.0f;
//	float camroll = acos(l);
//	if(mat->right.z < 0.0f)
//		camroll = -camroll;

	skyTop = currentSkyTop;
	skyBot = currentSkyBot;

	if(mat->up.z < -0.9f){
		skyTop = { 50, 50, 50, 255 };
		skyBot = { 50, 50, 50, 255 };
		renderQuad(0.0f, 0.0f,
				sk::globals.width, 0.0f,
				0.0f, sk::globals.height,
				sk::globals.width, sk::globals.height,
				skyTop, skyTop, skyBot, skyBot);
	}else{
		horizonz = calcHorizonCoords();
		float gradheight = sk::globals.height/2.0f;
		float topedge = horizonz - gradheight;
		float toppos, botpos;
		rw::RGBA gradtop = skyTop;
		rw::RGBA gradbot = skyBot;
		// The gradient
		if(horizonz > 0.0f && topedge < sk::globals.height){
			if(horizonz < sk::globals.height)
				botpos = horizonz;
			else{
				float f = (horizonz - sk::globals.height)/gradheight;
				gradbot.red = skyTop.red*f + (1.0f-f)*skyBot.red;
				gradbot.green = skyTop.green*f + (1.0f-f)*skyBot.green;
				gradbot.blue = skyTop.blue*f + (1.0f-f)*skyBot.blue;
				botpos = sk::globals.height;
			}
			if(topedge >= 0.0f)
				toppos = topedge;
			else{
				float f = (0.0f - topedge)/gradheight;
				gradtop.red = skyBot.red*f + (1.0f-f)*skyTop.red;
				gradtop.green = skyBot.green*f + (1.0f-f)*skyTop.green;
				gradtop.blue = skyBot.blue*f + (1.0f-f)*skyTop.blue;
				toppos = 0.0f;
			}
			renderQuad(0.0f, toppos,
				sk::globals.width, toppos,
				0.0f, botpos,
				sk::globals.width, botpos,
				gradtop, gradtop, gradbot, gradbot);
		}
		renderQuad(0.0f, horizonz,
			sk::globals.width, horizonz,
			0.0f, horizonz+SMALLSTRIPHEIGHT,
			sk::globals.width, horizonz+SMALLSTRIPHEIGHT,
			currentFog, currentFog, currentFog, currentFog);
		// Only top
		if(topedge > 0.0f){
			if(topedge > sk::globals.height)
				botpos = sk::globals.height;
			else
				botpos = topedge;
			renderQuad(0.0f, 0.0f,
				sk::globals.width, 0.0f,
				0.0f, botpos,
				sk::globals.width, botpos,
				skyTop, skyTop, skyTop, skyTop);
		}

//		renderQuad(0.0f, 0.0f,
//			sk::globals.width, 0.0f,
//			0.0f, sk::globals.height,
//			sk::globals.width, sk::globals.height,
//			top, top, bot, bot);
	}
}

void
drawHorizon(void)
{
	float gradheight = sk::globals.height/448.0f * HORIZSTRIPHEIGHT;
	skyBot.alpha = 230;
	skyTop.alpha = 80;

	bgcolor.red = 100;
	bgcolor.green = 100;
	bgcolor.blue = 100;
	bgcolor.alpha = 255;

	const float z1 = horizonz;
	const float z2 = z1 + SMALLSTRIPHEIGHT;
	const float z3 = z2 + gradheight;

	renderQuad(0.0f, z1,
		sk::globals.width, z1,
		0.0f, z2,
		sk::globals.width, z2,
		currentFog, currentFog, currentFog, currentFog);

	renderQuad(0.0f, z2,
		sk::globals.width, z2,
		0.0f, z3,
		sk::globals.width, z3,
		currentFog, currentFog, bgcolor, bgcolor);

	renderQuad(0.0f, z3,
		sk::globals.width, z3,
		0.0f, sk::globals.height,
		sk::globals.width, sk::globals.height,
		bgcolor, bgcolor, bgcolor, bgcolor);
}

void
Draw(void)
{
	static rw::RGBA clearcol = { 0x80, 0x80, 0x80, 0xFF };

	CPad *pad = CPad::GetPad(0);
	if(CPad::IsKeyDown('Q') || CPad::IsKeyDown(KEY_ESC) ||
	   pad->NewState.start && pad->NewState.select){
		sk::globals.quit = 1;
		return;
	}
	if(CPad::IsKeyJustDown('J')){
		curSectY--;
		if(curSectY < 0) curSectY = NUMSECTORSY-1;
	}
	if(CPad::IsKeyJustDown('K')){
		curSectY++;
		if(curSectY >= NUMSECTORSY) curSectY = 0;
	}
	if(CPad::IsKeyJustDown('H')){
		curSectX--;
		if(curSectX < 0) curSectX = NUMSECTORSX-1;
	}
	if(CPad::IsKeyJustDown('L')){
		curSectX++;
		if(curSectX >= NUMSECTORSX) curSectX = 0;
	}
	if(CPad::IsKeyJustDown('I')){
		curIntr++;
		if(curIntr >= gLevel->chunk->numInteriors) curIntr = -1;
		debug("interior: %d\n", curIntr);
	}
	if(CPad::IsKeyJustDown('U')){
		curIntr--;
		if(curIntr < -1) curIntr = gLevel->chunk->numInteriors-1;
		debug("interior: %d\n", curIntr);
	}
	if(CPad::IsKeyJustDown('T')){
		curHour--;
		if(curHour < 0) curHour = 23;
		debug("hour: %d\n", curHour);
	}
	if(CPad::IsKeyJustDown('Y')){
		curHour++;
		if(curHour >= 24) curHour = 0;
		debug("hour: %d\n", curHour);
	}
	if(CPad::IsKeyJustDown('Z')){
		curWeather--;
		if(curWeather < -1) curWeather = 7;
		debug("weather: %d\n", curWeather);
	}
	if(CPad::IsKeyJustDown('X')){
		curWeather++;
		if(curWeather >= 8) curWeather = -1;
		debug("weather: %d\n", curWeather);
	}
	if(CPad::IsKeyJustDown('C'))
		drawCubes = !drawCubes;
	if(CPad::IsKeyJustDown('B'))
		drawLOD = !drawLOD;

	updateTimecycle();

	CPad::UpdatePads();
	TheCamera.Process();

	TheCamera.m_rwcam->clear(&clearcol, rw::Camera::CLEARIMAGE|rw::Camera::CLEARZ);
	TheCamera.update();
	TheCamera.m_rwcam->beginUpdate();

	pAmbient->setColor(currentEmissive.red/255.0f, currentEmissive.green/255.0f, currentEmissive.blue/255.0f);

	DefinedState();
	rw::SetRenderState(rw::FOGENABLE, 0);

	drawBackground();
	drawHorizon();

	if(drawCubes)
		Renderer::renderCubesIPL();
//	renderCubesSector(curSectX, curSectY);

	rw::SetRenderState(rw::FOGENABLE, 1);

	int i;
	Renderer::reset();
//	renderSector(worldSectors[curSectX][curSectY]);
	if(curIntr >= 0)
		renderSector(&gLevel->sectors[gLevel->chunk->interiors[curIntr].sectorId]);
	else
		for(i = 0; i < gLevel->numWorldSectors; i++)
			renderSector(&gLevel->sectors[i]);

	Renderer::renderOpaque();
	Renderer::renderTransparent();

	CWaterLevel_::mspInst->RenderWater();

	TheCamera.m_rwcam->endUpdate();
	TheCamera.m_rwcam->showRaster();

	frameCounter++;
}

void
Idle(void)
{
	static int state = 0;
	switch(state){
	case 0:
		InitGame();
//dumpInstances();
//exit(0);
		state = 1;
		break;
	case 1:
		Draw();
		break;
	}
}


sk::EventStatus
AppEventHandler(sk::Event e, void *param)
{
	using namespace sk;
	Rect *r;
	switch(e){
	case INITIALIZE:
//		AllocConsole();
//		freopen("CONIN$", "r", stdin);
//		freopen("CONOUT$", "w", stdout);
//		freopen("CONOUT$", "w", stderr);

		Init();
		plAttachInput();
		return EVENTPROCESSED;
	case RWINITIALIZE:
		return ::InitRW() ? EVENTPROCESSED : EVENTERROR;
	case PLUGINATTACH:
		return attachPlugins() ? EVENTPROCESSED : EVENTERROR;
	case KEYDOWN:
		CPad::tempKeystates[*(int*)param] = 1;
		return EVENTPROCESSED;
	case KEYUP:
		CPad::tempKeystates[*(int*)param] = 0;
		return EVENTPROCESSED;
	case RESIZE:
		r = (Rect*)param;
		sk::globals.width = r->w;
		sk::globals.height = r->h;
		if(Scene.camera){
			sk::CameraSize(Scene.camera, r);
			TheCamera.m_aspectRatio = (float)r->w/r->h;
		}
		break;
	case IDLE:
		Idle();
		return EVENTPROCESSED;
	}
	return sk::EVENTNOTPROCESSED;
}
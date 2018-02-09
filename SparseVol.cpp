#include "SparseVol.h"
#include "app/GLInclude.h"
#include <iostream>

using namespace nvdb;
using namespace std;

SparseVol::SparseVol(): _initialized(false), _tex(0) {

}

SparseVol::~SparseVol() {
	if(_tex)
		glDeleteTextures ( 1, &_tex );
}

void SparseVol::init(int w, int h) {

	_width = w;
	_height = h;

	// Initialize GVDB
	int devid = -1;
	gvdb.SetVerbose ( true );
	gvdb.SetCudaDevice ( devid );
	gvdb.Initialize ();
	gvdb.AddPath ( std::string("/home/toand/git/gvdb-voxels/source/shared_assets/") );
	//gvdb.AddPath ( std::string(ASSET_PATH) );


	// Load VBX
	char scnpath[1024];		
	if ( !gvdb.getScene()->FindFile ( "explosion.vbx", scnpath ) ) {
		cout << "Cannot find vbx file" << endl;
		exit(1);
	}
	printf ( "Loading VBX. %s\n", scnpath );
	gvdb.SetChannelDefault ( 16, 16, 16 );
	gvdb.LoadVBX ( scnpath );

	// Set volume params
	gvdb.getScene()->SetSteps ( .25, 16, .25 );				// Set raycasting steps
	gvdb.getScene()->SetExtinct ( -1.0f, 1.5f, 0.0f );		// Set volume extinction
	gvdb.getScene()->SetVolumeRange ( 0.1f, 0.0f, .1f );	// Set volume value range
	gvdb.getScene()->SetCutoff ( 0.005f, 0.01f, 0.0f );
	gvdb.getScene()->SetBackgroundClr ( 0.1f, 0.2f, 0.4f, 1.0 );
	gvdb.getScene()->LinearTransferFunc ( 0.00f, 0.25f, Vector4DF(0,0,0,0), Vector4DF(1,1,0,0.1f) );
	gvdb.getScene()->LinearTransferFunc ( 0.25f, 0.50f, Vector4DF(1,1,0,0.4f), Vector4DF(1,0,0,0.3f) );
	gvdb.getScene()->LinearTransferFunc ( 0.50f, 0.75f, Vector4DF(1,0,0,0.3f), Vector4DF(.2f,.2f,0.2f,0.1f) );
	gvdb.getScene()->LinearTransferFunc ( 0.75f, 1.00f, Vector4DF(.2f,.2f,0.2f,0.1f), Vector4DF(0,0,0,0.0) );
	gvdb.CommitTransferFunc ();

	// Create Camera 
	Camera3D* cam = new Camera3D;						
	cam->setFov ( 50.0 );
	cam->setOrbit ( Vector3DF(20,30,0), Vector3DF(125,160,125), 700, 1.0 );	
	gvdb.getScene()->SetCamera( cam );
	
	// Create Light
	Light* lgt = new Light;								
	lgt->setOrbit ( Vector3DF(299,57.3f,0), Vector3DF(132,-20,50), 200, 1.0 );
	gvdb.getScene()->SetLight ( 0, lgt );	

	// Add render buffer
	cout << "Creating screen buffer:" << w << " x " << h << endl;
	gvdb.AddRenderBuf ( 0, w, h, 4 );
}


static const char *g_screenquad_vert = 
	"#version 440 core\n"
	"layout(location = 0) in vec3 vertex;\n"
	"layout(location = 1) in vec3 normal;\n"
	"layout(location = 2) in vec3 texcoord;\n"
	"uniform vec4 uCoords;\n"  
	"uniform vec2 uScreen;\n"  
	"out vec3 vtexcoord;\n"
	"void main() {\n"
	"   vtexcoord = texcoord*0.5+0.5;\n"
	"   gl_Position = vec4( (uCoords.x*2.0/uScreen.x)-1.0+(vertex.x+1.0)*uCoords.z/uScreen.x, (uCoords.y*2.0/uScreen.y)+1.0-(1.0-vertex.y)*uCoords.w/uScreen.y, 0.0f, 1.0f );\n"						    
	"}\n";

static const char *g_screenquad_frag = 
	"#version 440\n"	
	"uniform sampler2D uTex;\n"
	"in vec3 vtexcoord;\n"
	"out vec4 outColor;\n"
	"void main() {\n"
    "   outColor = vec4( texture ( uTex, vtexcoord.xy).xyz, 1 );\n"	
	"}\n";

void SparseVol::initScreenQuadGL ()
{
	int status;
	int maxLog = 65536, lenLog;
	char log[65536];

	// Create a screen-space shader
	m_screenquad_prog = (int) glCreateProgram ();	
	GLuint vShader = (int) glCreateShader ( GL_VERTEX_SHADER );
	glShaderSource ( vShader , 1, (const GLchar**) &g_screenquad_vert, NULL );
	glCompileShader ( vShader );
	glGetShaderiv( vShader, GL_COMPILE_STATUS, &status );
	if (!status) { 
		glGetShaderInfoLog ( vShader, maxLog, &lenLog, log );		
		//nvprintf ("*** Compile Error in init_screenquad vShader\n"); 
		//nvprintf ("  %s\n", log );	
		cout <<  "Compile Error in init_screenquad vShader" << endl;		
	}	

	GLuint fShader = (int) glCreateShader ( GL_FRAGMENT_SHADER );
	glShaderSource ( fShader , 1, (const GLchar**) &g_screenquad_frag, NULL );
	glCompileShader ( fShader );
	glGetShaderiv( fShader, GL_COMPILE_STATUS, &status );
	if (!status) {
		glGetShaderInfoLog ( vShader, maxLog, &lenLog, log );		
		//nvprintf ("*** Compile Error in init_screenquad fShader\n"); 
		//nvprintf ("  %s\n", log );	
		cout <<  "Compile Error in init_screenquad fShader" << endl;			
	}
	glAttachShader ( m_screenquad_prog, vShader );
	glAttachShader ( m_screenquad_prog, fShader );
	glLinkProgram ( m_screenquad_prog );		  
    glGetProgramiv( m_screenquad_prog, GL_LINK_STATUS, &status );
    if ( !status ) { 
		//nvprintf("*** Error! Failed to link in init_screenquad\n");
		cout << "Error! Failed to link in init_screenquad" << endl;
	}
	
	// Get texture parameter
	m_screenquad_utex = glGetUniformLocation ( m_screenquad_prog, "uTex" );
	m_screenquad_ucoords = glGetUniformLocation ( m_screenquad_prog, "uCoords" );
	m_screenquad_uscreen = glGetUniformLocation ( m_screenquad_prog, "uScreen" );

	// Create a screen-space quad VBO
	std::vector<nvVertex> verts;
	std::vector<nvFace> faces;
	verts.push_back ( nvVertex(-1,-1,0, -1,1,0) );
	verts.push_back ( nvVertex( 1,-1,0,  1,1,0) );
	verts.push_back ( nvVertex( 1, 1,0,  1,-1,0) );
	verts.push_back ( nvVertex(-1, 1,0, -1,-1,0) );
	faces.push_back ( nvFace(0,1,2) );
	faces.push_back ( nvFace(2,3,0) );

	glGenBuffers ( 1, (GLuint*) &m_screenquad_vbo[0] );
	glGenBuffers ( 1, (GLuint*) &m_screenquad_vbo[1] );
	//checkGL ( "glGenBuffers (init_screenquad)" );
	glGenVertexArrays ( 1, (GLuint*) &m_screenquad_vbo[2] );
	glBindVertexArray ( m_screenquad_vbo[2] );
	//checkGL ( "glGenVertexArrays (init_screenquad)" );
	glBindBuffer ( GL_ARRAY_BUFFER, m_screenquad_vbo[0] );
	glBufferData ( GL_ARRAY_BUFFER, verts.size() * sizeof(nvVertex), &verts[0].x, GL_STATIC_DRAW_ARB);		
	//checkGL ( "glBufferData[V] (init_screenquad)" );
	glVertexAttribPointer ( 0, 3, GL_FLOAT, false, sizeof(nvVertex), 0 );				// pos
	glVertexAttribPointer ( 1, 3, GL_FLOAT, false, sizeof(nvVertex), (void*) 12 );	// norm
	glVertexAttribPointer ( 2, 3, GL_FLOAT, false, sizeof(nvVertex), (void*) 24 );	// texcoord
	glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, m_screenquad_vbo[1] );
	glBufferData ( GL_ELEMENT_ARRAY_BUFFER, faces.size()*3*sizeof(int), &faces[0].a, GL_STATIC_DRAW_ARB);
	//checkGL ( "glBufferData[F] (init_screenquad)" );
	glBindVertexArray ( 0 );
}

void SparseVol::setup() {

	if(_initialized)
		return;

	// create a 2d texture to store rendered frame
	glGenTextures ( 1, &_tex );
	glBindTexture ( GL_TEXTURE_2D, _tex );
	glPixelStorei ( GL_UNPACK_ALIGNMENT, 4 );	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);	
	glTexImage2D  ( GL_TEXTURE_2D, 0, GL_RGBA8, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);	
	glBindTexture ( GL_TEXTURE_2D, 0 );

	// init quad to render
	initScreenQuadGL();

	_initialized = true;
}

void SparseVol::renderScreenQuadGL ( ) {
	// Prepare pipeline
	glDisable ( GL_DEPTH_TEST );
	glDisable ( GL_CULL_FACE );	
	glDepthMask ( GL_FALSE );
	// Select shader	
	glBindVertexArray ( m_screenquad_vbo[2] );
	glUseProgram ( m_screenquad_prog );
	//checkGL ( "glUseProgram" );
	// Select VBO	
	glBindBuffer ( GL_ARRAY_BUFFER, m_screenquad_vbo[0] );		
	glVertexAttribPointer ( 0, 3, GL_FLOAT, false, sizeof(nvVertex), 0 );		
	glVertexAttribPointer ( 1, 3, GL_FLOAT, false, sizeof(nvVertex), (void*) 12 );		
	glVertexAttribPointer ( 2, 3, GL_FLOAT, false, sizeof(nvVertex), (void*) 24 );
	glEnableVertexAttribArray ( 0 );	
	glEnableVertexAttribArray ( 1 );
	glEnableVertexAttribArray ( 2 );
	glBindBuffer ( GL_ELEMENT_ARRAY_BUFFER, m_screenquad_vbo[1] );	
	//checkGL ( "glBindBuffer" );
	// Select texture
	glEnable ( GL_TEXTURE_2D );
	glProgramUniform1i ( m_screenquad_prog, m_screenquad_utex, 0 );
	glProgramUniform4f ( m_screenquad_prog, m_screenquad_ucoords, 0, 0, (float)_width, (float)_height );
	glProgramUniform2f ( m_screenquad_prog, m_screenquad_uscreen, (float)_width, (float)_height );
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, _tex );
	// Draw
	glDrawElementsInstanced ( GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);	
	//checkGL ( "glDraw" );
	glUseProgram ( 0 );
	
	glDepthMask ( GL_TRUE );
}

void SparseVol::render(const float pos[3]) {
	setup();

	Camera3D* cam = gvdb.getScene()->getCamera();
	cam->setPos(pos[0], pos[1], pos[2]);

	// Render volume
	//gvdb.TimerStart ();
	gvdb.Render ( 0, SHADE_VOLUME, 0, 0, 1, 1, 1 );
	//float rtime = gvdb.TimerStop();
	//nvprintf ( "Render volume. %6.3f ms\n", rtime );

	// Copy render buffer into opengl texture
	// This function does a gpu-gpu device copy from the gvdb cuda output buffer
	// into the opengl texture, avoiding the cpu readback found in ReadRenderBuf
	gvdb.ReadRenderTexGL ( 0, _tex );

	// draw
	renderScreenQuadGL();

	cout << "." << std::flush;
}
#include<Windows.h>
#include<glew.h>               // This SHOULD be above GL.h header.
#include<gl/GL.h>

#include<stdio.h>
#include"vmath.h"
#include"ktx_loading_header.h"

#pragma comment(lib,"opengl32.lib")//for linking
#pragma comment(lib,"glew32.lib")//for linking


int Win_Width = 800;
int Win_Height = 600;
using namespace vmath;

static GLfloat angle = 0.0f;
GLuint gShaderProgramObject;
GLuint samplerUniform;
GLuint dmap_depthUniform;
GLuint tex_color;
double current_Time;
GLfloat gWidth, gHeight;
enum
{
	AMC_ATTRIBUTE_POSITION = 0,
	AMC_ATTRIBUTE_COLOR,
	AMC_ATTRIBUTE_NORMAL,
	AMC_ATTRIBUTE_TEXCOORD0
};

//
GLuint vao;
GLuint vbo;
GLuint mvpUniform;
GLuint projectionUniform;
GLuint modelViewUniform;
mat4 perspectiveProjectionMatrix;

GLuint gNumberOfSegmentUniform; //MultipleLines -> Strips  | One Line divided into Multiple parts ->Segments
GLuint gNumberOfStripsUniform;
GLuint gLineColorUniform;
unsigned int gNumberOfLineSegments;

//

HDC ghdc = NULL;
HGLRC ghglrc = NULL;
HWND ghwnd;
HWND hwnd;
FILE *gpLogFile = NULL;
WINDOWPLACEMENT gwpprev = { sizeof(WINDOWPLACEMENT) };
DWORD gdwStyle = 0;
bool gbIsActiveWindow = false;
bool gbIsFullScreen = false;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ToggleFullScreen(void);
void update(void);
void uninitialize(void);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int iCmdShow)
{
	int initialize(void);

	//Variable Declarations
	bool bIsDone = false;
	int iRet = 0;
	TCHAR szAppName[] = TEXT("perspective");
	WNDCLASSEX wndclass;
	MSG msg;
	void display(void);

	if (fopen_s(&gpLogFile, "Log.TXT", "w") != 0)
	{
		MessageBox(NULL, TEXT("LOG FILE WAS NOT CREATED"), TEXT("ERROR"), MB_OK);
		exit(0);
	}
	else
	{
		fprintf_s(gpLogFile, "Log File Created\n");
	}

	//andclassinitialization
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;// | CS_OWNDC;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.lpfnWndProc = WndProc;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszClassName = szAppName;
	wndclass.lpszMenuName = NULL;
	wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (RegisterClassEx(&wndclass) == 0)
	{
		//Failed to register wndclassex
	}

	//CreateWindow
	hwnd = CreateWindowEx(WS_EX_APPWINDOW,
		szAppName,
		TEXT("OrthoTriangle"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		100,
		100,
		Win_Width, Win_Height,
		NULL,
		NULL,
		hInstance,
		0);

	ghwnd = hwnd;

	iRet = initialize();
	if (iRet == -1)
	{
		fprintf_s(gpLogFile, "choosePixekFormat() Failed\n");
		DestroyWindow(hwnd);
	}
	else if (iRet == -2)
	{
		fprintf_s(gpLogFile, "SetPixelFormat() Failed\n");
		DestroyWindow(hwnd);
	}
	else if (iRet == -3)
	{
		fprintf_s(gpLogFile, "wglCreateContext() Failed\n");
		DestroyWindow(hwnd);
	}
	else if (iRet == -4)
	{
		fprintf_s(gpLogFile, "wglMakeCurrent() Failed\n");
		DestroyWindow(hwnd);
	}
	else
	{
		fprintf_s(gpLogFile, "Initialization successfull\n");
	}

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);
	SetFocus(hwnd);

	//gameloop
	while (bIsDone == false)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				bIsDone = true;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			if (gbIsActiveWindow == true)
			{
				//Here actually play the game. 

			}
			/*Here actually call Display,As this is Double Buffer Program. No need of WM_PAINT*/

			display();
		}
	}
	return((int)msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	void resize(int, int);


	switch (iMsg)
	{
	case WM_CREATE:

		break;

	case WM_SETFOCUS:
		gbIsActiveWindow = true;
		break;

	case WM_KILLFOCUS:
		gbIsActiveWindow = false;
		break;

	case WM_SIZE:
		resize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_ERASEBKGND:
		return(0);

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_KEYDOWN:

		switch (wParam)
		{
		case VK_ESCAPE:
			ToggleFullScreen();

			DestroyWindow(hwnd);
			break;

		case 'F':
		case 'f':
			ToggleFullScreen();
			break;


		case VK_UP:
			gNumberOfLineSegments++;
			if (gNumberOfLineSegments >= 50)
				gNumberOfLineSegments = 50;

			break;

		case VK_DOWN:
			gNumberOfLineSegments <= 0;
			gNumberOfLineSegments = 1;

			break;
		}
		break;

	case WM_DESTROY:
		uninitialize();
		PostQuitMessage(0);
		break;
	}
	return(DefWindowProc(hwnd, iMsg, wParam, lParam));
}

int initialize(void)
{
	GLuint gVertexShaderObject;
	GLuint gFragmentShaderObject;
	GLuint gTesselationControlShaderObject;
	GLuint gTesselationEvaluationShaderObject;

	GLenum result;
	void resize(int, int);

	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex;

	memset((void*)&pfd, NULL, sizeof(PIXELFORMATDESCRIPTOR));
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 32;
	ghdc = GetDC(ghwnd);

	iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);
	if (iPixelFormatIndex == 0)
	{
		return(-1);
	}

	if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == FALSE)
	{
		return(-2);
	}

	ghglrc = wglCreateContext(ghdc);

	if (ghglrc == NULL)
	{
		return(-3);
	}

	if (wglMakeCurrent(ghdc, ghglrc) == FALSE)
	{
		return(-4);
	}
	result = glewInit();
	if (result != GLEW_OK)
	{
		fprintf(gpLogFile, "ERROR : glewInit FAILED!!!\n");
		uninitialize();
		DestroyWindow(hwnd);
	}



	//////////////////////////////// V E R T E X - S H A D E R //////////////////////////


	//Define Vertex Shader Object
	gVertexShaderObject = glCreateShader(GL_VERTEX_SHADER); //This command will create the Shader Object
	//Now Write vertex shader code
	const GLchar **p;
	const GLchar *vertexShaderSourceCode =

		"#version 430 core" \
		"\n"
		"out VS_OUT" \
		"{" \
			"vec2 tc;" \
		"}vs_out;" \

		"void main(void)" \
		"{" \
		"const vec4 vertices[] = vec4[](vec4(-0.5,0.0,-0.5,1.0),vec4(0.5,0.0,-0.5,1.0),vec4(-0.5,0.0,0.5,1.0),vec4(0.5,0.0,0.5,1.0));" \
		"int x = gl_InstanceID & 63;" \
		"int y = gl_InstanceID >> 6;" \
		"vec2 offs = vec2(x,y);" \
		"vs_out.tc = (vertices[gl_VertexID].xz + offs + vec2(0.5)) / 64.0;" \
		"gl_Position = vertices[gl_VertexID] + vec4(float(x - 32),0.0,float(y - 32),0.0);" \
		"}";
	// GPU will run the above code. And GPU WILL RUN FOR PER VERTEX. If there are 1000 vertex. Then GPU will run this shader for
	//1000 times. We are Multiplying each vertex with the Model View Matrix.
	//And how does the GPU gets to know about at what offset the array has to be taken . Go to glVertexAttribPointer() in Display.
	// in = Input. 

	//p = &vertexShaderSourceCode;
		//Specify above source code to the vertex shader object
	glShaderSource(gVertexShaderObject, 1, (const GLchar **)&vertexShaderSourceCode, NULL);

	//Compile the vertex shader 
	glCompileShader(gVertexShaderObject);

	//////////////// Error Checking//////////////////
	//Code for catching the errors 
	GLint iShaderCompileStatus = 0;
	GLint iInfoLogLength = 0;
	GLchar *szInfoLog = NULL;


	glGetShaderiv(gVertexShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(gVertexShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (GLchar *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written;
				glGetShaderInfoLog(gVertexShaderObject, iInfoLogLength, &written, szInfoLog);
				fprintf(gpLogFile, "  VERTEX SHADER :   %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				DestroyWindow(hwnd);
				exit(0);


			}
		}
	}

	////////////////////// TESSELLATION CONTROL SHADER //////////////////////////
	//This shader will create patch of 4 vertices
	gTesselationControlShaderObject = glCreateShader(GL_TESS_CONTROL_SHADER);
	const GLchar *tessellationControlShaderSourceCode =
		"#version 430" \
		"\n" \
		"layout(vertices=4)out;" \
	
		
		"in VS_OUT" \
		"{" \
		"vec2 tc;" \
		"} tcs_in[];" \
	
		
		"out TCS_OUT" \
		"{" \
			"vec2 tc;" \
		"} tcs_out[];" \
		
		
		"uniform mat4 u_mvp_matrix;" \
		"void main(void)" \
		"{" \
		"if(gl_InvocationID == 0)" \
		"{" \
			"vec4 p0 =  u_mvp_matrix * gl_in[0].gl_Position;" \
			"vec4 p1 =  u_mvp_matrix * gl_in[1].gl_Position;" \
			"vec4 p2 =  u_mvp_matrix * gl_in[2].gl_Position;" \
			"vec4 p3 =  u_mvp_matrix * gl_in[3].gl_Position;" \
			"p0 /=p0.w;" \
			"p1 /=p1.w;" \
			"p2 /=p2.w;" \
			"p3 /=p3.w;" \

			"if(p0.z<=0.0||" \
			"p1.z<=0.0||" \
			"p2.z<=0.0||" \
			"p3.z<=0.0)" \
			"{" \
				"gl_TessLevelOuter[0] = 0.0;" \
				"gl_TessLevelOuter[1] = 0.0;" \
				"gl_TessLevelOuter[2] = 0.0;" \
				"gl_TessLevelOuter[3] = 0.0;" \
			"}" \
			"else" \
			"{" \
				"float l0 =length(p2.xy - p0.xy) * 100.0 + 1.0;" \
				"float l1 =length(p3.xy - p2.xy) * 100.0 + 1.0;" \
				"float l2 =length(p3.xy - p1.xy) * 100.0 + 1.0;" \
				"float l3 =length(p1.xy - p0.xy) * 100.0 + 1.0;" \
				"gl_TessLevelOuter[0] = l0;" \
				"gl_TessLevelOuter[1] = l1;" \
				"gl_TessLevelOuter[2] = l2;" \
				"gl_TessLevelOuter[3] = l3;" \
				"gl_TessLevelInner[0] = min(l1,l3);" \
				"gl_TessLevelInner[1] = min(l0,l2);" \

			"}" \
		"}" \

		"gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;" \
		"tcs_out[gl_InvocationID].tc = tcs_in[gl_InvocationID].tc;" \
		"}";



	glShaderSource(gTesselationControlShaderObject, 1, (const GLchar **)&tessellationControlShaderSourceCode, NULL);

	//compileShader
	glCompileShader(gTesselationControlShaderObject);

	//////////////// Error Checking//////////////////
	//Code for catching the errors 
	iShaderCompileStatus = 0;
	iInfoLogLength = 0;
	szInfoLog = NULL;


	glGetShaderiv(gTesselationControlShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(gTesselationControlShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (GLchar *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written9;
				glGetShaderInfoLog(gTesselationControlShaderObject, iInfoLogLength, &written9, szInfoLog);
				fprintf(gpLogFile, "TCS :  %s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				DestroyWindow(hwnd);
				exit(0);


			}
		}
	}

	////////////////// TESSELLATION EVLAUTATION SHADER /////////////

	gTesselationEvaluationShaderObject = glCreateShader(GL_TESS_EVALUATION_SHADER);

	const GLchar *tessellationEvalutationShaderSourceCode =
		"#version 430" \
		"\n" \
		"layout(quads,fractional_odd_spacing)in;" \
		
		"uniform mat4 u_mvp_matrix;" \
		"uniform mat4 u_p_matrix;" \
		"uniform mat4 u_modelView_matrix;" \
		"uniform sampler2D tex_displacement;" \
		"uniform float dmap_depth;" \
		
		"in TCS_OUT" \
		"{" \
			"vec2 tc;" \
		"}tes_in[];" \
		
		
		"out TES_OUT" \
		"{" \
		"vec2 tc;" \
		"vec3 world_coord;" \
		"vec3 eye_coord;" \
		"}tes_out;" \

		"void main(void)" \
		"{" \
		"vec2 tc1 = mix(tes_in[0].tc,tes_in[1].tc,gl_TessCoord.x);" \
		"vec2 tc2 = mix(tes_in[2].tc,tes_in[3].tc,gl_TessCoord.x);" \
		"vec2 tc = mix(tc2,tc1,gl_TessCoord.y);" \

		"vec4 p1 = mix(gl_in[0].gl_Position,gl_in[1].gl_Position,gl_TessCoord.x);" \
		"vec4 p2 = mix(gl_in[2].gl_Position,gl_in[3].gl_Position,gl_TessCoord.x);" \
		"vec4 p = mix(p2,p1,gl_TessCoord.y);" \
		"p.y += texture(tex_displacement,tc).r * dmap_depth;" \

		"vec4 P_eye = u_modelView_matrix * p;" \

		"tes_out.tc = tc;" \
		"tes_out.world_coord = p.xyz;" \
		"tes_out.eye_coord = P_eye.xyz;" \
		"gl_Position = u_p_matrix * P_eye;" \
		
		"}";

	glShaderSource(gTesselationEvaluationShaderObject, 1, (const GLchar **)&tessellationEvalutationShaderSourceCode, NULL);

	//compileShader
	glCompileShader(gTesselationEvaluationShaderObject);

	//////////////// Error Checking//////////////////
	//Code for catching the errors 
	iShaderCompileStatus = 0;
	iInfoLogLength = 0;
	szInfoLog = NULL;


	glGetShaderiv(gTesselationEvaluationShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(gTesselationEvaluationShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (GLchar *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written6;
				glGetShaderInfoLog(gTesselationEvaluationShaderObject, iInfoLogLength, &written6, szInfoLog);
				fprintf(gpLogFile, "TES :%s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				DestroyWindow(hwnd);
				exit(0);


			}
		}
	}







	/////////////////    F R A G M E N T S H A D E R            //////////////////////////
	//Define Vertex Shader Object
	gFragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER); //This command will create the Shader Object
	//Now Write vertex shader code
	const GLchar *fragmentShaderSourceCode =
		"#version 430 core" \
		"\n" \
		"layout(binding = 1) uniform sampler2D tex_color;" \
		"uniform bool enable_fog = true;" \
		"uniform vec4 fog_color = vec4(0.7,0.8,0.9,0.0);" \
		"in TES_OUT" \
		"{" \
		"vec2 tc;" \
		"vec3 world_coord;" \
		"vec3 eye_coord;" \
		"}fs_in;" \
		"out vec4 FragColor;" \

		"vec4 fog(vec4 c)" \
		"{" \
		"float z = length(fs_in.eye_coord);" \
		"float de = 0.025 * smoothstep(0.0,6.0,10.0 - fs_in.world_coord.y);" \
		"float di = 0.045 * (smoothstep(0.0,40.0,20.0 - fs_in.world_coord.y));" \
		"float extinction = exp(-z * de);" \
		"float inscattering = exp(-z * di);" \
		"return c * extinction + fog_color * (1.0 - inscattering); " \
		"}" \

		"void main(void)" \
		"{" \
		"vec4 landscape = texture(tex_color,fs_in.tc);" \
		"if(enable_fog)" \
		"{" \

			"FragColor = fog(landscape);" \

		"}" \
		"else" \
		"{" \
			"FragColor =  landscape;" \
		"}" \
		"}";

	//FragColor = vec4(1,1,1,1) = White Color
	//this means here we are giving color to the Triangle.
	//Specify above source code to the vertex shader object
	glShaderSource(gFragmentShaderObject, 1, (const GLchar **)&fragmentShaderSourceCode, NULL);

	//Compile the vertex shader 
	glCompileShader(gFragmentShaderObject);
	//Code for catching the errors 
		   /*iShaderCompileStatus = 0;
		   iInfoLogLength = 0;*/
	szInfoLog = NULL;


	glGetShaderiv(gFragmentShaderObject, GL_COMPILE_STATUS, &iShaderCompileStatus);
	if (iShaderCompileStatus == GL_FALSE)
	{
		glGetShaderiv(gFragmentShaderObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (GLchar *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written1;
				glGetShaderInfoLog(gFragmentShaderObject, iInfoLogLength, &written1, szInfoLog);
				fprintf(gpLogFile, "FRAGMENT :%s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				DestroyWindow(hwnd);
				exit(0);


			}
		}
	}
	// CREATE SHADER PROGRAM OBJECT
	gShaderProgramObject = glCreateProgram();
	//attach vertex shader to the gShaderProgramObject
	glAttachShader(gShaderProgramObject, gVertexShaderObject);
	glAttachShader(gShaderProgramObject, gTesselationControlShaderObject);
	glAttachShader(gShaderProgramObject, gTesselationEvaluationShaderObject);
	glAttachShader(gShaderProgramObject, gFragmentShaderObject);


	//Pre-Linking  binding to vertexAttributes
	glBindAttribLocation(gShaderProgramObject, AMC_ATTRIBUTE_POSITION, "vPosition");
	//glBindAttribLocation(gShaderProgramObject, AMC_, "vPosition");
	//Here the above line means that we are linking the GPU's variable vPosition with the CPU's  enum member  i.e AMC_ATTRIBUTE_POSITION .
	//So whatever changes will be done in AMC_ATTRIBUTE_POSITION , those will also reflect in vPosition

	//RULE : ALWAYS BIND THE ATTRIBUTES BEFORE LINKING AND BIND THE UNIFORM AFTER LINKING.

	//Link the shader program 
	glLinkProgram(gShaderProgramObject);

	//Code for catching the errors 
	GLint iProgramLinkStatus = 0;



	glGetProgramiv(gShaderProgramObject, GL_LINK_STATUS, &iProgramLinkStatus);
	if (iProgramLinkStatus == GL_FALSE)
	{
		glGetProgramiv(gShaderProgramObject, GL_INFO_LOG_LENGTH, &iInfoLogLength);
		if (iInfoLogLength > 0)
		{
			szInfoLog = (GLchar *)malloc(iInfoLogLength);
			if (szInfoLog != NULL)
			{
				GLsizei written3;
				glGetProgramInfoLog(gShaderProgramObject, iInfoLogLength, &written3, szInfoLog);
				fprintf(gpLogFile, "%s\n", szInfoLog);
				free(szInfoLog);
				uninitialize();
				DestroyWindow(hwnd);
				exit(0);


			}
		}
	}


	//POST Linking
	//Retrieving uniform locations 
	mvpUniform = glGetUniformLocation(gShaderProgramObject, "u_mvp_matrix");
	projectionUniform = glGetUniformLocation(gShaderProgramObject, "u_p_matrix");
	modelViewUniform = glGetUniformLocation(gShaderProgramObject, "u_modelView_matrix");
	dmap_depthUniform = glGetUniformLocation(gShaderProgramObject, "dmap_depth");
	samplerUniform = glGetUniformLocation(gShaderProgramObject, "tex_displacement");
	tex_color = glGetUniformLocation(gShaderProgramObject, "tex_color");
	//Here we have done all the preparations of data transfer from CPU to GPU

	const GLfloat Vertices[] =
	{
		-1.0f,-1.0f,-0.5f,1.0f,0.5f,-1.0f,1.0f,1.0f
	};
	//Basically we are giving the vertices coordinates in the above array

	//Create vao - Vertex array objects
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	//in the above statement we have accesed the GL-ARRAY_BUFFER using vbo. Without it wouldn't be possible to get GL_ARRAY_BUFFER
	//For the sake of understanding . GL_ARRAY_BUFFER is in the GPU side ad=nd we have bind our CPU side vbo with it like a Pipe to get the access.
	glBufferData(GL_ARRAY_BUFFER, 8  * sizeof(Vertices), Vertices, GL_STATIC_DRAW);
	//The above statement states that , we are passing our vertices array to the GPU and GL_STATIC_DRAW means draw it now only. Don't draw it in Runtime. 
	//The below statement states that after storing the data in the GPU'S buffer . We are passing it to the vPosition now . 

	glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	//GL_FALSE = We are not giving normalized coordinates as our coordinates are not converted in 0 - 1 range.
	//3 = This is the thing I was talking about in initialize. Here, we are telling GPU to break our array in 3 parts . 
	//0 and Null are for the Interleaved. 
	//GL_FLOAT- What is the type? .
	//AMC_ATTRIBUTE_POSITION. here we are passing data to vPosition. 

	glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//Depth Lines
	glClearDepth(1.0f);
	glClearColor(0.0f, 0.25f, 0.0f, 0.0f);
	glLineWidth(3.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	samplerUniform = load("terragen1.ktx",0);
	glActiveTexture(GL_TEXTURE1);
	tex_color = load("terragen_color.ktx", 0);

	//initializing Perspective ProjectionMatrix to identity.
	perspectiveProjectionMatrix = mat4::identity();
	gNumberOfLineSegments = 1;
	resize(Win_Width, Win_Height);

	return(0);
}

void resize(int width, int height)
{


	if (height == 0)
	{
		height = 1;
	}

	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	gWidth = width;
	gHeight = height;
	/*if (width < height)
	{
		orthographicProjectionMatrix = ortho(
			-100.0f, 100.0f,
			-100.0f * ((float)height / (float)width), 100.0f * ((float)height / (float)width),
			-100.0f, 100.0f);
	}
	else
	{
		orthographicProjectionMatrix = ortho(
			-100.0f * ((float)width / (float)height), 100.0f * ((float)width / (float)height),
			-100.0f, 100.0f,
			-100.0f, 100.0f);
	}*/
	perspectiveProjectionMatrix = perspective(45.0f,
		(GLfloat)width / (GLfloat)height,
		0.1f,
		100.0f);

}



void display(void)
{


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	static const GLfloat black[] = {0.85f,0.95f,1.0f,1.0f};
	static const GLfloat One = 1.0f;
	static double  last_Time = 0.0f;
	static double  total_Time = 0.0f;

	total_Time += (current_Time - last_Time);
	last_Time = current_Time;

	current_Time = current_Time + 0.001f;

	float t = (float)total_Time * 0.3f;
	float r = sinf(t * 5.37f) * 15.0f - 19.0f;
	float h = cosf(t * 4.79f) * 2.0f + 3.2f;

	glViewport(0, 0, gWidth, gHeight);
	glClearBufferfv(GL_COLOR, 0, black);
	glClearBufferfv(GL_DEPTH, 0, &One);





	// use shader program
	glUseProgram(gShaderProgramObject);

	//declaration of matrices
	mat4 modelViewMatrix;
	mat4 modelViewProjectionMatrix;
	mat4 projectionMatrix;
	// intialize above matrices to identity
	modelViewMatrix = mat4::identity();
	modelViewProjectionMatrix = mat4::identity();
	projectionMatrix = mat4::identity();

	// perform necessary transformations
	// modelViewMatrix = vmath::lookat(vec3(sinf(t) * r,h,cosf(t) * r),vec3(0.0f),vec3(0.0f,1.0f,0.0f));
	modelViewMatrix = vmath::lookat(vec3(0.0f,0.0f,5.0f), vec3(0.0f), vec3(0.0f, -1.0f, 0.0f));
	projectionMatrix = perspective(60.0f, (GLfloat)gWidth / (GLfloat)gHeight, 0.1f, 1000.0f);
	
	// do necessary matrix multiplication
	// this was internally done by glOrtho() in FFP.	

	modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;

	// send necessary matrices to shader in respective uniforms
	glUniformMatrix4fv(mvpUniform, 1, GL_FALSE, modelViewProjectionMatrix);
	glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, modelViewMatrix);
	glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, projectionMatrix);
	glUniform1f(dmap_depthUniform,4.0f);

	//GL_FALSE = Should we transpose the matrix?
	//DirectX is roj major so there we will have to transpose but OpenGL is Colomn major so no need to transpose. 

	
	TCHAR str[255];
	wsprintf(str, TEXT("OGL Programmable Pipeline Window : [Segments = %d]"), gNumberOfLineSegments);
	SetWindowText(hwnd, str);
	/*glUniform1i(gNumberOfStripsUniform, 1);
	glUniform4fv(gLineColorUniform, 1, vmath::vec4(1.0f, 1.0f, 0.0f, 1.0f));
*/

	// bind with vao
	//this will avoid many binding to vbo
	glBindVertexArray(vao);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glPatchParameteri(GL_PATCH_VERTICES, 4);
	// bind with textures

	// draw necessary scene
	glDrawArraysInstanced(GL_PATCHES, 0, 4,64 * 64);
	//  0 =  From where to start in the array. 
	// We have to start from 0th element i.e 0. if the 0th element would be 50.0f then we would have given the 2nd parameter as 50.
	//3 = How many Vertices? 
	//GL_TRIANGLES is the thing between glBegin() and glEnd()


	// unbind vao
	glBindVertexArray(0);

	// unuse program
	glUseProgram(0);
	SwapBuffers(ghdc);

}

void uninitialize(void)
{
	if (vbo)
	{
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}

	if (vao)
	{
		glDeleteBuffers(1, &vao);
		vao = 0;
	}



	if (gShaderProgramObject)
	{
		GLsizei shaderCount;
		GLsizei shaderNumber;

		glUseProgram(gShaderProgramObject);
		glGetProgramiv(gShaderProgramObject, GL_ATTACHED_SHADERS, &shaderCount);

		GLuint *pShaders = (GLuint *)malloc(sizeof(GLuint) * shaderCount);
		if (pShaders)
		{
			glGetAttachedShaders(gShaderProgramObject, shaderCount, &shaderCount, pShaders);

			for (shaderNumber = 0; shaderNumber < shaderCount; shaderNumber++)
			{
				// detach shader
				glDetachShader(gShaderProgramObject, pShaders[shaderNumber]);

				// delete shader
				glDeleteShader(pShaders[shaderNumber]);
				pShaders[shaderNumber] = 0;
			}
			free(pShaders);
		}

		glDeleteProgram(gShaderProgramObject);
		gShaderProgramObject = 0;
		glUseProgram(0);

	}
	if (gbIsFullScreen == true)
	{
		SetWindowLong(ghwnd, GWL_STYLE, gdwStyle | WS_OVERLAPPEDWINDOW);

		SetWindowPlacement(ghwnd, &gwpprev);

		SetWindowPos(ghwnd,
			HWND_TOP,
			0,
			0,
			0,
			0,
			SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
		ShowCursor(TRUE);
	}

	if (wglGetCurrentContext() == ghglrc)
	{
		wglMakeCurrent(NULL, NULL);
		if (ghglrc)
		{
			wglDeleteContext(ghglrc);
			ghglrc = NULL;
		}
	}

	if (ghdc)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	if (gpLogFile)
	{
		fprintf_s(gpLogFile, "Log File Closed");
		fclose(gpLogFile);
	}
}


void ToggleFullScreen()
{
	MONITORINFO MI;

	if (gbIsFullScreen == false)
	{
		gdwStyle = GetWindowLong(ghwnd, GWL_STYLE);

		if (gdwStyle & WS_OVERLAPPEDWINDOW)
		{
			MI = { sizeof(MONITORINFO) };

			if (
				GetWindowPlacement(ghwnd, &gwpprev)
				&&
				GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &MI)
				)
			{
				SetWindowLong(ghwnd, GWL_STYLE, gdwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd, HWND_TOP,
					MI.rcMonitor.left,
					MI.rcMonitor.top,
					MI.rcMonitor.right - MI.rcMonitor.left,
					MI.rcMonitor.bottom - MI.rcMonitor.top,
					SWP_NOZORDER | WS_OVERLAPPED);
			}
		}
		ShowCursor(FALSE);
		gbIsFullScreen = true;
	}
	else
	{
		SetWindowLong(ghwnd, GWL_STYLE, gdwStyle | WS_OVERLAPPEDWINDOW);

		SetWindowPlacement(ghwnd, &gwpprev);


		SetWindowPos(ghwnd,
			HWND_TOP,
			0,
			0,
			0,
			0,
			SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);
		ShowCursor(TRUE);
		gbIsFullScreen = false;
	}

}

void update(void)
{


}
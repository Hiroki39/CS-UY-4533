/*
    hw02.cpp
    CS-UY 4533, Nov 3rd, 2020
    Hongyi Zheng
    Rolling Sphere
*/
#include <fstream>
#include <vector>
#include "Angel-yjc.hpp"
using namespace std;

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

GLuint program;       /* shader program object id */
GLuint sphere_buffer; /* vertex buffer object id for sphere */
GLuint floor_buffer;  /* vertex buffer object id for floor */
GLuint axis_buffer;

// Projection transformation parameters
GLfloat fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat aspect;       // Viewport aspect ratio
GLfloat zNear = 0.5, zFar = 20.0;

GLfloat angle = 0.0;                  // rotation angle in degrees
vec4 init_eye(7.0, 3.0, -10.0, 1.0);  // initial viewer position
vec4 eye = init_eye;                  // current viewer position

// 1: animation; 0: non-animation. Toggled by key 'a' or 'A'
int animationFlag = 0;

// 1: solid sphere; 0: wireframe sphere. Toggled by key 'c' or 'C'
int sphereFlag = 0;

// 1: solid floor; 0: wireframe floor. Toggled by key 'f' or 'F'
int floorFlag = 1;

bool b_hit = false;

vector<vec3> sphere_points;  // positions for all vertices
vector<vec3> sphere_colors;  // colors for all vertices

vector<vec3> floor_points;  // positions for all vertices
vector<vec3> floor_colors;  // colors for all vertices

vector<vec3> axis_points;
vector<vec3> axis_colors;

mat4 accumulated_rotation(1.0);
vector<vec3> rolling_points{vec3(-4, 1, 4), vec3(3, 1, -4), vec3(-3, 1, -3)};
int num_pts = rolling_points.size();
int curr_pt_index = 0;

// generate 2 triangles: 6 vertices
void floor() {
    floor_points.emplace_back(5, 0, 8);
    floor_points.emplace_back(5, 0, -4);
    floor_points.emplace_back(-5, 0, -4);
    floor_points.emplace_back(-5, 0, -4);
    floor_points.emplace_back(-5, 0, 8);
    floor_points.emplace_back(5, 0, 8);
    for (int i = 0; i < 6; ++i) {
        floor_colors.emplace_back(0, 1, 0);
    }
}

// generate 3 axis: 6 vertices
void axis() {
    // x-axis
    axis_points.emplace_back(0, 0, 0);
    axis_colors.emplace_back(1, 0, 0);
    axis_points.emplace_back(10, 0, 0);
    axis_colors.emplace_back(1, 0, 0);

    // y-axis
    axis_points.emplace_back(0, 0, 0);
    axis_colors.emplace_back(1, 0, 1);
    axis_points.emplace_back(0, 10, 0);
    axis_colors.emplace_back(1, 0, 1);

    // z-axis
    axis_points.emplace_back(0, 0, 0);
    axis_colors.emplace_back(0, 0, 1);
    axis_points.emplace_back(0, 0, 10);
    axis_colors.emplace_back(0, 0, 1);
}

void read_file() {
    ifstream sphereStream;  // create a filestream
    sphereStream.clear();
    string filename;
    cout << "Enter file name: ";  // prompt user enter the filename
    cin >> filename;
    sphereStream.open(filename);

    // check if the file is succesfully opened
    if (!sphereStream) {
        cerr << "failed to open " << filename << endl;
        exit(1);
    }

    int num_triangles;
    sphereStream >> num_triangles;

    GLfloat x, y, z;
    int num_vertices;

    // record the maximum absolute coorinate
    for (int i = 0; i < num_triangles; ++i) {
        sphereStream >> num_vertices;
        for (int i = 0; i < num_vertices; ++i) {
            sphereStream >> x >> y >> z;
            sphere_points.emplace_back(x, y, z);
            sphere_colors.emplace_back(1.0, 0.84, 0);
        }
    }
    sphereStream.close();
}

// OpenGL initialization
void init() {
    read_file();
    // Create and initialize a vertex buffer object for sphere, to be used in
    // display()
    glGenBuffers(1, &sphere_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);

    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec3) * (sphere_points.size() + sphere_colors.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * sphere_points.size(),
                    sphere_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3) * sphere_points.size(),
                    sizeof(vec3) * sphere_colors.size(), sphere_colors.data());

    floor();
    // Create and initialize a vertex buffer object for floor, to be used in
    // display()
    glGenBuffers(1, &floor_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec3) * (floor_points.size() + floor_colors.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * floor_points.size(),
                    floor_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3) * floor_points.size(),
                    sizeof(vec3) * floor_colors.size(), floor_colors.data());

    axis();
    // Create and initialize a vertex buffer object for axis, to be used in
    // display()
    glGenBuffers(1, &axis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec3) * (axis_points.size() + axis_colors.size()), NULL,
                 GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * axis_points.size(),
                    axis_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3) * axis_points.size(),
                    sizeof(vec3) * axis_colors.size(), axis_colors.data());

    // Load shaders and create a shader program (to be used in display())
    program = InitShader("vshader42.vert", "fshader42.frag");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.529, 0.807, 0.92, 0.0);
    glLineWidth(2.0);
}

// draw the object that is associated with the vertex buffer object "buffer"
// and has "num_vertices" vertices.
void drawObj(GLuint buffer, int num_vertices) {
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(vec3) * num_vertices));
    // the offset is the (total) size of the previous vertex attribute array(s)

    /* Draw a sequence of geometric objs (triangles) from the vertex buffer
       (using the attributes specified in each enabled vertex attribute array)
     */
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vColor);
}

// draw the object that is associated with the vertex buffer object "buffer"
// and has "num_vertices" vertices.
void drawAxis(GLuint buffer, int num_vertices) {
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(vec3) * num_vertices));
    // the offset is the (total) size of the previous vertex attribute array(s)

    /* Draw a sequence of geometric objs (lines) from the vertex buffer
       (using the attributes specified in each enabled vertex attribute array)
     */
    glDrawArrays(GL_LINES, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vColor);
}

void display(void) {
    GLuint model_view;  // model-view matrix uniform shader variable location
    GLuint projection;  // projection matrix uniform shader variable location

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);  // Use the shader program

    model_view = glGetUniformLocation(program, "model_view");
    projection = glGetUniformLocation(program, "projection");

    /*---  Set up and pass on Projection matrix to the shader ---*/
    mat4 p = Perspective(fovy, aspect, zNear, zFar);
    // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(projection, 1, GL_TRUE, p);

    /*---  Set up and pass on Model-View matrix to the shader ---*/
    // eye is a global variable of vec4 set to init_eye and updated by
    // keyboard()
    vec4 at(0.0, 0.0, 0.0, 1.0);
    vec4 up(0.0, 1.0, 0.0, 0.0);

    mat4 mv = LookAt(eye, at, up);

    /*----- Set up the Mode-View matrix for the floor -----*/
    // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
    if (floorFlag == 1)  // Filled floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else  // Wireframe floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    drawObj(floor_buffer, floor_points.size());  // draw the floor

    glLineWidth(4.0);
    drawAxis(axis_buffer, axis_points.size());  // draw the axis

    /*----- Set Up the Model-View matrix for the sphere -----*/
    int next_pt_index = (curr_pt_index + 1) % num_pts;
    vec3 direction = normalize(rolling_points[next_pt_index] -
                               rolling_points[curr_pt_index]);
    vec3 rotation_axis = cross(vec3(0, 1, 0), direction);
    GLfloat distance = 2 * M_PI * angle / 360;
    vec3 from_curr_pt = direction * distance;
    // The set-up below gives a new scene (scene 2), using
    // Correct LookAt().
    mv = mv * Translate(rolling_points[curr_pt_index] + from_curr_pt) *
         Rotate(angle, rotation_axis[0], rotation_axis[1], rotation_axis[2]) *
         accumulated_rotation;

    // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv);
    if (sphereFlag == 1)  // Filled sphere
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else  // Wireframe sphere
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0);
    drawObj(sphere_buffer, sphere_points.size());  // draw the sphere

    glutSwapBuffers();
}

void idle(void) {
    angle += 1.5;
    GLfloat distance = 2 * M_PI * angle / 360;
    int next_pt_index = (curr_pt_index + 1) % num_pts;
    if (distance >=
        length(rolling_points[next_pt_index] - rolling_points[curr_pt_index])) {
        vec3 direction = normalize(rolling_points[next_pt_index] -
                                   rolling_points[curr_pt_index]);
        vec3 rotation_axis = cross(vec3(0, 1, 0), direction);
        accumulated_rotation = Rotate(angle, rotation_axis[0], rotation_axis[1],
                                      rotation_axis[2]) *
                               accumulated_rotation;
        curr_pt_index = next_pt_index;
        angle = 0;
    }
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 033:  // Escape Key
        case 'q':
        case 'Q':
            exit(EXIT_SUCCESS);
            break;

        case 'X':
            eye[0] += 1.0;
            break;
        case 'x':
            eye[0] -= 1.0;
            break;
        case 'Y':
            eye[1] += 1.0;
            break;
        case 'y':
            eye[1] -= 1.0;
            break;
        case 'Z':
            eye[2] += 1.0;
            break;
        case 'z':
            eye[2] -= 1.0;
            break;

        case 'b':
        case 'B':  // Toggle between animation and non-animation
            if (!b_hit) b_hit = true;
            animationFlag = 1 - animationFlag;
            if (animationFlag == 1)
                glutIdleFunc(idle);
            else
                glutIdleFunc(NULL);
            break;

        case 'c':
        case 'C':  // Toggle between filled and wireframe sphere
            sphereFlag = 1 - sphereFlag;
            break;

        case 'f':
        case 'F':  // Toggle between filled and wireframe floor
            floorFlag = 1 - floorFlag;
            break;

        case ' ':  // reset to initial viewer/eye position
            eye = init_eye;
            break;
    }
    glutPostRedisplay();
}

void menu(int id) {
    if (id == 1) {
        eye = init_eye;
    } else {
        exit(EXIT_SUCCESS);
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        glutCreateMenu(menu);
        glutAddMenuEntry("Default View Point", 1);
        glutAddMenuEntry("Quit", 2);
        glutAttachMenu(GLUT_LEFT_BUTTON);
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        if (b_hit) {
            animationFlag = 1 - animationFlag;
            if (animationFlag == 1)
                glutIdleFunc(idle);
            else
                glutIdleFunc(NULL);
        }
    }
}

void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    aspect = (GLfloat)width / (GLfloat)height;
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
#ifdef __APPLE__  // Enable core profile of OpenGL 3.2 on macOS.
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH |
                        GLUT_3_2_CORE_PROFILE);
#else
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif
    glutInitWindowSize(512, 512);
    glutCreateWindow("Rolling Sphere");

#ifdef __APPLE__  // on macOS
    // Core profile requires to create a Vertex Array Object (VAO).
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
#else  // on Linux or Windows, we still need glew
    /* Call glewInit() and error checking */
    int err = glewInit();
    if (GLEW_OK != err) {
        printf("Error: glewInit failed: %s\n", (char*)glewGetErrorString(err));
        exit(1);
    }
#endif

    // Get info of GPU and supported OpenGL version
    cout << "Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "OpenGL version supported " << glGetString(GL_VERSION) << endl;

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);

    init();
    glutMainLoop();
    return 0;
}

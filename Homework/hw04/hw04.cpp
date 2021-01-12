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

#define NumParticle 1000
#define ImageWidth 32
#define ImageHeight 32
GLubyte Image[ImageHeight][ImageWidth][4];

#define stripeImageWidth 32
GLubyte stripeImage[4 * stripeImageWidth];

GLuint program1, program2;  // shader program object id
GLuint texNames[2];
GLuint sphere_buffer, floor_buffer, axis_buffer, shadow_buffer, particle_buffer;

// Projection transformation parameters
GLfloat fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat aspect;       // Viewport aspect ratio
GLfloat zNear = 0.5, zFar = 20.0;

GLfloat angle = 0.0;                  // rotation angle in degrees
vec4 init_eye(7.0, 3.0, -10.0, 1.0);  // initial viewer position
vec4 eye = init_eye;                  // current viewer position

// boolean and integer flags
bool animation = false;   // Toggled by key 'a' or 'A'
bool solidSphere = true;  // Toggled by key 'c' or 'C'
bool solidFloor = true;   // Toggled by key 'f' or 'F'
bool blending = true;
bool shadow = true;
bool vertical = true;
bool upright = true;
bool object_space = true;
bool lattice = false;
bool firework = false;

GLuint ground_texture = 1;  // 0 - no texture; 1 - 2D texture for ground
GLuint sphere_texture = 0;  // 0 - no texture; 2 - 1D texture for sphere;
                            // 3 - 2D texture for sphere;
GLuint fog = 0;

bool smoothShading = true;
bool spotlight = false;
bool lighting = true;
bool b_key_pressed = false;

// storage data structures
vector<vec4> sphere_points;   // positions for all sphere vertices
vector<vec4> sphere_colors;   // colors for all sphere vertices
vector<vec3> sphere_normals;  // normals for all sphere vertices

vector<vec4> floor_points;     // positions for all floor vertices
vector<vec4> floor_colors;     // colors for all floor vertices
vector<vec3> floor_normals;    // normals for all floor vertices
vector<vec2> floor_texcoords;  // texture coordinates for all floor vertices

vector<vec4> axis_points;  // positions for all axis vertices
vector<vec4> axis_colors;  // colors for all axis vertices

vector<vec4> shadow_colors;  // positions for all shadow vertices

vector<vec4> particle_colors;      // colors for all particle vertices
vector<vec4> particle_velocities;  // velocities for all particle vertices

vec4 material_ambient;
vec4 material_diffuse;
vec4 material_specular;

mat3 normal_matrix;
// In World frame. Needs to transform it to Eye Frame before sending it to the
// shader(s).
vec4 light_position(-14.0, 12.0, -3.0, 1.0);

GLuint t_sub;

mat4 accumulated_rotation(1.0);
vector<vec4> rolling_pts{vec4(-4, 1, 4, 1), vec4(3, 1, -4, 1),
                         vec4(-3, 1, -3, 1)};
int num_pts = rolling_pts.size();
int curr_pt_index = 0;

// generate 2 triangles: 6 vertices
void floor() {
    floor_points.emplace_back(5, 0, 8, 1);
    floor_texcoords.emplace_back(6, 5);

    floor_points.emplace_back(5, 0, -4, 1);
    floor_texcoords.emplace_back(0, 5);

    floor_points.emplace_back(-5, 0, -4, 1);
    floor_texcoords.emplace_back(0, 0);

    floor_points.emplace_back(-5, 0, -4, 1);
    floor_texcoords.emplace_back(0, 0);

    floor_points.emplace_back(-5, 0, 8, 1);
    floor_texcoords.emplace_back(6, 0);

    floor_points.emplace_back(5, 0, 8, 1);
    floor_texcoords.emplace_back(6, 5);

    for (int i = 0; i < floor_points.size(); ++i) {
        floor_colors.emplace_back(0, 1, 0, 1);
        floor_normals.emplace_back(0, 1, 0);
    }
}

// generate 3 axis: 6 vertices
void axis() {
    // x-axis
    axis_points.emplace_back(0, 0, 0, 1);
    axis_colors.emplace_back(1, 0, 0, 1);
    axis_points.emplace_back(10, 0, 0, 1);
    axis_colors.emplace_back(1, 0, 0, 1);

    // y-axis
    axis_points.emplace_back(0, 0, 0, 1);
    axis_colors.emplace_back(1, 0, 1, 1);
    axis_points.emplace_back(0, 10, 0, 1);
    axis_colors.emplace_back(1, 0, 1, 1);

    // z-axis
    axis_points.emplace_back(0, 0, 0, 1);
    axis_colors.emplace_back(0, 0, 1, 1);
    axis_points.emplace_back(0, 0, 10, 1);
    axis_colors.emplace_back(0, 0, 1, 1);
}

void particles() {
    for (int i = 0; i < NumParticle; ++i) {
        particle_velocities.emplace_back(2.0 * ((rand() % 256) / 256.0 - 0.5),
                                         1.2 * 2.0 * ((rand() % 256) / 256.0),
                                         2.0 * ((rand() % 256) / 256.0 - 0.5));
        particle_colors.emplace_back((rand() % 256) / 256.0,
                                     (rand() % 256) / 256.0,
                                     (rand() % 256) / 256.0, 1);
    }
}

void read_file() {
    ifstream sphereStream;  // create a filestream
    sphereStream.clear();
    char selection;
    string filename;

    // prompt user to select a file
    cout << "Select Number of Vertices of the Sphere" << endl;
    cout << "a) 8" << endl;
    cout << "b) 128" << endl;
    cout << "c) 256" << endl;
    cout << "d) 1024" << endl;
    cout << "Enter your choice: ";
    cin >> selection;
    switch (selection) {
        case 'a':
        case 'A':
            sphereStream.open("sphere.8");
            break;
        case 'b':
        case 'B':
            sphereStream.open("sphere.128");
            break;
        case 'c':
        case 'C':
            sphereStream.open("sphere.256");
            break;
        case 'd':
        case 'D':
            sphereStream.open("sphere.1024");
            break;
        default:
            cout << "Invalid Option" << endl;
            exit(1);
            break;
    }

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
            sphere_points.emplace_back(x, y, z, 1);
            sphere_colors.emplace_back(1.0, 0.84, 0, 1);
            shadow_colors.emplace_back(0.25, 0.25, 0.25, 0.65);
        }
        vec4 vert1 = sphere_points[sphere_points.size() - 3];
        vec4 vert2 = sphere_points[sphere_points.size() - 2];
        vec4 vert3 = sphere_points[sphere_points.size() - 1];
        vec3 normal = normalize(cross(vert2 - vert1, vert3 - vert1));
        sphere_normals.insert(sphere_normals.end(), num_vertices, normal);
    }
    sphereStream.close();
}

// Set up parameters that are uniform variables in shader and won't change
// through out the display() function
void initializeUniformVars(mat4 mv) {
    /*----- Shader Constant Parameters -----*/
    glUniform4fv(glGetUniformLocation(program1, "GlobalAmbient"), 1,
                 vec4(1.0, 1.0, 1.0, 1.0));
    glUniform4fv(glGetUniformLocation(program1, "PositionalAmbient"), 1,
                 vec4(0.0, 0.0, 0.0, 1.0));
    glUniform4fv(glGetUniformLocation(program1, "PositionalDiffuse"), 1,
                 vec4(1.0, 1.0, 1.0, 1.0));
    glUniform4fv(glGetUniformLocation(program1, "PositionalSpecular"), 1,
                 vec4(1.0, 1.0, 1.0, 1.0));
    glUniform4fv(glGetUniformLocation(program1, "DistantAmbient"), 1,
                 vec4(0.0, 0.0, 0.0, 1.0));
    glUniform4fv(glGetUniformLocation(program1, "DistantDiffuse"), 1,
                 vec4(0.8, 0.8, 0.8, 1.0));
    glUniform4fv(glGetUniformLocation(program1, "DistantSpecular"), 1,
                 vec4(0.2, 0.2, 0.2, 1.0));

    glUniform1f(glGetUniformLocation(program1, "ConstAtt"), 2.0);
    glUniform1f(glGetUniformLocation(program1, "LinearAtt"), 0.01);
    glUniform1f(glGetUniformLocation(program1, "QuadAtt"), 0.001);
    glUniform1f(glGetUniformLocation(program1, "Shininess"), 125.0);
    glUniform1f(glGetUniformLocation(program1, "SpotlightExponent"), 15.0);

    vec4 spotlight_destination(-6.0, 0.0, -4.5, 1.0);

    // The Light Position and Spotlight Destination in Eye Frame
    vec4 light_position_eyeFrame = mv * light_position;
    vec4 spotlight_destination_eyeFrame = mv * spotlight_destination;
    glUniform4fv(glGetUniformLocation(program1, "LightPosition"), 1,
                 light_position_eyeFrame);
    glUniform4fv(glGetUniformLocation(program1, "SpotlightDestination"), 1,
                 spotlight_destination_eyeFrame);
    glUniform4fv(glGetUniformLocation(program1, "DistantLightDirection"), 1,
                 vec4(0.1, 0.0, -1.0, 0.0));

    glUniform4fv(glGetUniformLocation(program1, "FogColor"), 1,
                 vec4(0.7, 0.7, 0.7, 0.5));
    glUniform1f(glGetUniformLocation(program1, "FogStart"), 0.0);
    glUniform1f(glGetUniformLocation(program1, "FogEnd"), 18.0);
    glUniform1f(glGetUniformLocation(program1, "FogDensity"), 0.09);
    glUniform1i(glGetUniformLocation(program1, "Fog"), fog);

    glUniform1i(glGetUniformLocation(program1, "GroundTexture"), 0);
    glUniform1i(glGetUniformLocation(program1, "StripeTexture"), 1);

    glUniform1i(glGetUniformLocation(program1, "Spotlight"), spotlight);
    glUniform1i(glGetUniformLocation(program1, "Vertical"), vertical);
    glUniform1i(glGetUniformLocation(program1, "Upright"), upright);
    glUniform1i(glGetUniformLocation(program1, "ObjSpace"), object_space);
}

// Set up uniform variables in shader that will change throughout display()
// function
// Note: "LightPosition" in shader must be in the Eye Frame.
//       So we use parameter "mv", the model-view matrix, to transform
//       light_position to the Eye Frame.
void updateUniformVars() {
    glUniform4fv(glGetUniformLocation(program1, "MaterialAmbient"), 1,
                 material_ambient);
    glUniform4fv(glGetUniformLocation(program1, "MaterialDiffuse"), 1,
                 material_diffuse);
    glUniform4fv(glGetUniformLocation(program1, "MaterialSpecular"), 1,
                 material_specular);

    glUniformMatrix3fv(glGetUniformLocation(program1, "Normal_Matrix"), 1,
                       GL_TRUE, normal_matrix);
}

/*************************************************************
void image_set_up(void):
  generate checkerboard and stripe images.

* Inside init(), call this function and set up texture objects
  for texture mapping.
  (init() is called from main() before calling glutMainLoop().)
***************************************************************/
void image_set_up() {
    int i, j, c;

    /* --- Generate checkerboard image to the image array ---*/
    for (i = 0; i < ImageHeight; i++)
        for (j = 0; j < ImageWidth; j++) {
            c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0));

            if (c == 1) {  // white
                c = 255;
                Image[i][j][0] = (GLubyte)c;
                Image[i][j][1] = (GLubyte)c;
                Image[i][j][2] = (GLubyte)c;
            } else {  // green
                Image[i][j][0] = (GLubyte)0;
                Image[i][j][1] = (GLubyte)150;
                Image[i][j][2] = (GLubyte)0;
            }

            Image[i][j][3] = (GLubyte)255;
        }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    /*--- Generate 1D stripe image to array stripeImage[] ---*/
    for (j = 0; j < stripeImageWidth; j++) {
        /* When j <= 4, the color is (255, 0, 0),   i.e., red stripe/line.
           When j > 4,  the color is (255, 255, 0), i.e., yellow remaining
           texture
         */
        stripeImage[4 * j] = (GLubyte)255;
        stripeImage[4 * j + 1] = (GLubyte)((j > 4) ? 255 : 0);
        stripeImage[4 * j + 2] = (GLubyte)0;
        stripeImage[4 * j + 3] = (GLubyte)255;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /*----------- End 1D stripe image ----------------*/

    /*--- texture mapping set-up is to be done in
          init() (set up texture objects),
          display() (activate the texture object to be used, etc.)
          and in shaders.
     ---*/

} /* end function */

// OpenGL initialization
void init() {
    read_file();
    image_set_up();

    /*--- Create and Initialize a texture object ---*/
    glGenTextures(1, texNames);  // Generate texture obj name(s)

    glActiveTexture(GL_TEXTURE0);  // Set the active texture unit to be 0
    glBindTexture(GL_TEXTURE_2D,
                  texNames[0]);  // Bind the texture to this texture unit

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImageWidth, ImageHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, Image);

    glActiveTexture(GL_TEXTURE1);  // Set the active texture unit to be 0
    glBindTexture(GL_TEXTURE_1D,
                  texNames[1]);  // Bind the texture to this texture unit

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, stripeImageWidth, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, stripeImage);

    // Create and initialize a vertex buffer object for sphere, to be used
    // in display()
    glGenBuffers(1, &sphere_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);

    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec4) * (sphere_points.size() + sphere_colors.size()) +
                     sizeof(vec3) * sphere_normals.size(),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * sphere_points.size(),
                    sphere_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * sphere_points.size(),
                    sizeof(vec4) * sphere_colors.size(), sphere_colors.data());
    glBufferSubData(
        GL_ARRAY_BUFFER,
        sizeof(vec4) * (sphere_points.size() + sphere_colors.size()),
        sizeof(vec3) * sphere_normals.size(), sphere_normals.data());

    floor();

    // Create and initialize a vertex buffer object for floor, to be used in
    // display()
    glGenBuffers(1, &floor_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec4) * (floor_points.size() + floor_colors.size()) +
                     sizeof(vec3) * floor_normals.size() +
                     sizeof(vec2) * floor_texcoords.size(),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * floor_points.size(),
                    floor_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * floor_points.size(),
                    sizeof(vec4) * floor_colors.size(), floor_colors.data());
    glBufferSubData(GL_ARRAY_BUFFER,
                    sizeof(vec4) * (floor_points.size() + floor_colors.size()),
                    sizeof(vec3) * floor_normals.size(), floor_normals.data());
    glBufferSubData(GL_ARRAY_BUFFER,
                    sizeof(vec4) * (floor_points.size() + floor_colors.size()) +
                        sizeof(vec3) * floor_normals.size(),
                    sizeof(vec2) * floor_texcoords.size(),
                    floor_texcoords.data());

    axis();

    // Create and initialize a vertex buffer object for axis, to be used in
    // display()
    glGenBuffers(1, &axis_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec4) * (axis_points.size() + axis_colors.size()), NULL,
                 GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * axis_points.size(),
                    axis_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * axis_points.size(),
                    sizeof(vec4) * axis_colors.size(), axis_colors.data());

    // Create and initialize a vertex buffer object for shadow, to be used in
    // display()
    glGenBuffers(1, &shadow_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, shadow_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec4) * (sphere_points.size() + shadow_colors.size()),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * sphere_points.size(),
                    sphere_points.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * sphere_points.size(),
                    sizeof(vec4) * shadow_colors.size(), shadow_colors.data());

    particles();

    // Create and initialize a vertex buffer object for particles, to be used in
    // display()
    glGenBuffers(1, &particle_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, particle_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vec4) * particle_colors.size() +
                     sizeof(vec3) * particle_velocities.size(),
                 NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec4) * particle_colors.size(),
                    particle_colors.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec4) * particle_colors.size(),
                    sizeof(vec3) * particle_velocities.size(),
                    particle_velocities.data());

    // Load shaders and create a shader program (to be used in display())
    program1 = InitShader("vshader53.vert", "fshader53.frag");
    program2 = InitShader("firework.vert", "firework.frag");

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.529, 0.807, 0.92, 0.0);
}

// draw the object that is associated with the vertex buffer object "buffer"
// and has "num_vertices" vertices.
// Mode: GL_POINTS, GL_LINES or GL_TRIANGLES
void drawObj(GLuint buffer, int num_vertices, GLenum mode,
             GLboolean objLighting = GL_FALSE,
             GLboolean objSmoothShading = GL_FALSE, GLuint objTexture = 0,
             GLboolean objLattice = GL_FALSE) {
    // Activate the vertex buffer object to be drawn
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    glUniform1i(glGetUniformLocation(program1, "Lighting"), objLighting);
    glUniform1i(glGetUniformLocation(program1, "SmoothShading"),
                objSmoothShading);
    glUniform1i(glGetUniformLocation(program1, "Texture"), objTexture);
    glUniform1i(glGetUniformLocation(program1, "Lattice"), objLattice);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(program1, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(program1, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(vec4) * num_vertices));

    GLuint vNormal = glGetAttribLocation(program1, "vNormal");
    glEnableVertexAttribArray(vNormal);
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(vec4) * num_vertices * 2));

    GLuint vTexCoord = glGetAttribLocation(program1, "vTexCoord");
    glEnableVertexAttribArray(vTexCoord);
    glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(vec4) * num_vertices * 2 +
                                        sizeof(vec3) * num_vertices));

    glDrawArrays(mode, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vColor);
    glDisableVertexAttribArray(vNormal);
    glDisableVertexAttribArray(vTexCoord);
}

void drawFirework(GLuint buffer, int num_vertices) {
    GLuint t = glutGet(GLUT_ELAPSED_TIME);
    glUniform1i(glGetUniformLocation(program2, "time"), (t - t_sub) % 10000);

    // Activate the vertex buffer object to be drawn
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vColor = glGetAttribLocation(program2, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    GLuint vVelocity = glGetAttribLocation(program2, "vVelocity");
    glEnableVertexAttribArray(vVelocity);
    glVertexAttribPointer(vVelocity, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(vec4) * num_vertices));

    glPointSize(3.0);
    glDrawArrays(GL_POINTS, 0, num_vertices);

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vColor);
    glDisableVertexAttribArray(vVelocity);
}

void display(void) {
    GLuint ModelView;   // model-view matrix uniform shader variable location
    GLuint Projection;  // projection matrix uniform shader variable location

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program1);  // Use the shader program

    ModelView = glGetUniformLocation(program1, "ModelView");
    Projection = glGetUniformLocation(program1, "Projection");

    /*---  Set up and pass on Projection matrix to the shader ---*/
    mat4 p = Perspective(fovy, aspect, zNear, zFar);
    // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(Projection, 1, GL_TRUE, p);

    /*---  Set up and pass on Model-View matrix to the shader ---*/
    // eye is a global variable of vec4 set to init_eye and updated by
    // keyboard()
    vec4 at(0.0, 0.0, 0.0, 1.0);
    vec4 up(0.0, 1.0, 0.0, 0.0);

    mat4 mv = LookAt(eye, at, up);

    initializeUniformVars(mv);  // Initialize uniform variable that will be
                                // constant throughout display() function

    drawObj(axis_buffer, axis_points.size(), GL_LINES);  // draw the axis

    /*----- Set Up the Model-View matrix for the sphere -----*/
    int next_pt_index = (curr_pt_index + 1) % num_pts;
    vec4 direction =
        normalize(rolling_pts[next_pt_index] - rolling_pts[curr_pt_index]);
    vec4 rotation_axis = vec4(cross(vec4(0, 1, 0, 0), direction), 0);
    GLfloat distance = 2 * M_PI * angle / 360;
    vec4 from_curr_pt = direction * distance;

    // The set-up below gives a new scene (scene 2), using correct LookAt().
    mat4 mv_sphere =
        mv * Translate(rolling_pts[curr_pt_index] + from_curr_pt) *
        Rotate(angle, rotation_axis[0], rotation_axis[1], rotation_axis[2]) *
        accumulated_rotation;

    // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv_sphere);

    // Set up the Normal Matrix from the model-view matrix
    normal_matrix = NormalMatrix(mv_sphere, 0);

    // Set up lighting parameters that are uniform variables in shader
    material_ambient = vec4(0.2, 0.2, 0.2, 1.0);
    material_diffuse = vec4(1.0, 0.84, 0.0, 1.0);
    material_specular = vec4(1.0, 0.84, 0.0, 1.0);

    // update lighting parameters and normal matrix
    updateUniformVars();

    if (solidSphere) {  // Filled sphere
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawObj(sphere_buffer, sphere_points.size(), GL_TRIANGLES, lighting,
                smoothShading, sphere_texture, lattice);
    } else {  // Wireframe sphere
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawObj(sphere_buffer, sphere_points.size(), GL_TRIANGLES);
    }

    /*----- Set up the Mode-View matrix for the floor -----*/
    // Draw floor for the first time, write to frame buffer only
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv);

    // Set up the Normal Matrix from the model-view matrix
    normal_matrix = NormalMatrix(mv, 0);

    // Set up lighting parameters that are uniform variables in shader
    material_ambient = vec4(0.2, 0.2, 0.2, 1.0);
    material_diffuse = vec4(0.0, 1.0, 0.0, 1.0);
    material_specular = vec4(0.0, 0.0, 0.0, 1.0);

    updateUniformVars();  // update lighting parameters and normal matrix

    glDepthMask(GL_FALSE);
    if (solidFloor) {  // Filled floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawObj(floor_buffer, floor_points.size(), GL_TRIANGLES, lighting,
                GL_FALSE, ground_texture);
    } else {  // Wireframe floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        drawObj(floor_buffer, floor_points.size(), GL_TRIANGLES);
    }
    if (!blending) glDepthMask(GL_TRUE);

    if (shadow && eye.y > 0) {
        /*----- Set Up the Model-View matrix for the shadow -----*/
        // GL_TRUE: matrix is row-major
        mat4 shadow_projection(light_position.y, 0, 0, 0, -light_position.x, 0,
                               -light_position.z, -1, 0, 0, light_position.y, 0,
                               0, 0, 0, light_position.y);

        mat4 mv_shadow = mv * shadow_projection *
                         Translate(rolling_pts[curr_pt_index] + from_curr_pt) *
                         Rotate(angle, rotation_axis[0], rotation_axis[1],
                                rotation_axis[2]) *
                         accumulated_rotation;

        glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv_shadow);

        if (solidSphere)  // Filled sphere
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        else  // Wireframe sphere
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        if (blending) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        drawObj(shadow_buffer, shadow_colors.size(), GL_TRIANGLES, GL_FALSE,
                GL_FALSE, 0, lattice);  // draw the shadow
        if (blending) glDisable(GL_BLEND);
    }

    if (blending) glDepthMask(GL_TRUE);

    // draw floor for the second time, write floor to z-buffer only
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, mv);
    if (solidFloor)  // Filled floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else  // Wireframe floor
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    drawObj(floor_buffer, floor_points.size(), GL_TRIANGLES);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if (firework) {
        glUseProgram(program2);  // Use the firework shader program

        glUniformMatrix4fv(glGetUniformLocation(program2, "Projection"), 1,
                           GL_TRUE, p);
        glUniformMatrix4fv(glGetUniformLocation(program2, "ModelView"), 1,
                           GL_TRUE, mv);
        drawFirework(particle_buffer, NumParticle);
    }

    glutSwapBuffers();
}

void idle(void) {
    angle += 2.0;
    GLfloat distance = 2 * M_PI * angle / 360;
    int next_pt_index = (curr_pt_index + 1) % num_pts;
    if (distance >=
        length(rolling_pts[next_pt_index] - rolling_pts[curr_pt_index])) {
        vec4 direction =
            normalize(rolling_pts[next_pt_index] - rolling_pts[curr_pt_index]);
        vec4 rotation_axis = vec4(cross(vec4(0, 1, 0, 0), direction), 0);
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
            if (!b_key_pressed) b_key_pressed = true;
            animation = !animation;
            if (animation)
                glutIdleFunc(idle);
            else
                glutIdleFunc(NULL);
            break;

        case 'c':
        case 'C':  // Toggle between filled and wireframe sphere
            solidSphere = !solidSphere;
            break;

        case 'f':
        case 'F':  // Toggle between filled and wireframe floor
            solidFloor = !solidFloor;
            break;

        case ' ':  // reset to initial viewer/eye position
            eye = init_eye;
            break;

        case 'v':
        case 'V':
            vertical = true;
            if (object_space) upright = true;
            break;

        case 's':
        case 'S':
            vertical = false;
            if (object_space) upright = false;
            break;

        case 'u':
        case 'U':
            upright = true;
            if (object_space) vertical = true;
            break;

        case 't':
        case 'T':
            upright = false;
            if (object_space) vertical = false;
            break;

        case 'o':
        case 'O':
            object_space = true;
            // make sure the keys are aligned when switching back
            upright = vertical;
            break;

        case 'e':
        case 'E':
            object_space = false;
            break;
    }
    glutPostRedisplay();
}

void mainMenu(int id) {
    switch (id) {
        case 0:
            eye = init_eye;
            break;
        case 1:
            exit(EXIT_SUCCESS);
            break;
        case 2:
            shadow = true;
            break;
        case 3:
            shadow = false;
            break;
        case 4:
            lighting = true;
            break;
        case 5:
            lighting = false;
            break;
        case 6:
            spotlight = true;
            break;
        case 7:
            spotlight = false;
            break;
        case 8:
            smoothShading = false;
            break;
        case 9:
            smoothShading = true;
            break;
        case 10:
            solidSphere = false;
            break;
        case 11:
            solidSphere = true;
            break;
        case 12:
            fog = 0;
            break;
        case 13:
            fog = 1;
            break;
        case 14:
            fog = 2;
            break;
        case 15:
            fog = 3;
            break;
        case 16:
            blending = true;
            break;
        case 17:
            blending = false;
            break;
        case 18:
            ground_texture = 1;
            break;
        case 19:
            ground_texture = 0;
            break;
        case 20:
            sphere_texture = 2;
            break;
        case 21:
            sphere_texture = 3;
            break;
        case 22:
            sphere_texture = 0;
            break;
        case 23:
            lattice = true;
            break;
        case 24:
            lattice = false;
            break;
        case 25:
            firework = true;
            t_sub = glutGet(GLUT_ELAPSED_TIME);
            break;
        case 26:
            firework = false;
            break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
        int shadow_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("On", 2);
        glutAddMenuEntry("Off", 3);

        int lighting_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("On", 4);
        glutAddMenuEntry("Off", 5);

        int lightsource_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Spotlight", 6);
        glutAddMenuEntry("Point Source", 7);

        int shading_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Flat Shading", 8);
        glutAddMenuEntry("Smooth Shading", 9);

        int wireframe_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Wireframe", 10);
        glutAddMenuEntry("Solid", 11);

        int fog_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("No Fog", 12);
        glutAddMenuEntry("Linear", 13);
        glutAddMenuEntry("Exponential", 14);
        glutAddMenuEntry("Exponential Square", 15);

        int blending_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Yes", 16);
        glutAddMenuEntry("No", 17);

        int ground_texture_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Yes", 18);
        glutAddMenuEntry("No", 19);

        int sphere_texture_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Yes - Contour Lines", 20);
        glutAddMenuEntry("Yes - Checkerboard", 21);
        glutAddMenuEntry("No", 22);

        int lattice_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Yes", 23);
        glutAddMenuEntry("No", 24);

        int firework_menu = glutCreateMenu(mainMenu);
        glutAddMenuEntry("Yes", 25);
        glutAddMenuEntry("No", 26);

        glutCreateMenu(mainMenu);
        glutAddMenuEntry("Default View Point", 0);

        glutAddSubMenu("Shadow", shadow_menu);
        glutAddSubMenu("Lighting", lighting_menu);
        glutAddSubMenu("Light Source", lightsource_menu);
        glutAddSubMenu("Shading", shading_menu);
        glutAddSubMenu("Sphere Type", wireframe_menu);
        glutAddSubMenu("Fog Effect", fog_menu);
        glutAddSubMenu("Blending Shadow", blending_menu);
        glutAddSubMenu("Texture Mapped Ground", ground_texture_menu);
        glutAddSubMenu("Texture Mapped Sphere", sphere_texture_menu);
        glutAddSubMenu("Lattice Effect", lattice_menu);
        glutAddSubMenu("Firework", firework_menu);
        glutAddMenuEntry("Quit", 1);

        glutAttachMenu(GLUT_LEFT_BUTTON);
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        if (b_key_pressed) {
            animation = !animation;
            if (animation)
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

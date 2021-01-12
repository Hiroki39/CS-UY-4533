/*
    hw01.cpp
    CS-UY 4533, Oct 6th, 2020
    Hongyi Zheng
    Circle Drawer
*/
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef __APPLE__  // include Mac OS X verions of headers
#include <GLUT/glut.h>
#else  // non-Mac OS X operating systems
#include <GL/glut.h>
#endif

using namespace std;

#define XOFF 50
#define YOFF 50
#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600
#define FRAME 100  // the total number of frames during the animation

int curr_frame = 1;
int option;                          // user option
vector<vector<int>> scaled_circles;  // scaled circles (in world coordinate)
vector<vector<int>> final_circles;   // circles that are ready to display

void display(void);
void myinit(void);
void idle(void);

void file_in(void);  // Function to handle file input
void circlePoint(int x, int y, int center_x, int center_y);
void drawCircle(int x, int y, int r);
void adjustScale(vector<vector<int>>& circles, int numerator, int denominator);
void toScreenCoordinate(vector<vector<int>>& circles);

int main(int argc, char** argv) {
    glutInit(&argc, argv);

    // Use both double buffering and Z buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    glutInitWindowPosition(XOFF, YOFF);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("CS6533/CS4533 Assignment 1");

    glutDisplayFunc(display);

    // prompt user to select an option
    cout << "Circle Drawer" << endl;
    cout << "1 Enter Parameter Manually" << endl;
    cout << "2 Read From File (No Animation)" << endl;
    cout << "3 Read From File (With Animation)" << endl;
    cout << "Enter an option (1-3): ";
    cin >> option;
    while (!((option == 1 || option == 2) || option == 3)) {
        cout << "Invalid input, try again: ";
        cin >> option;
    };

    if (option == 1) {
        int x, y, r;
        int num_circles;
        cout << "Enter number of circles: " << endl;
        cin >> num_circles;
        for (int i = 0; i < num_circles; ++i) {
            cout << "Enter parameters(x, y, r) in screen coordinate: ";
            cin >> x >> y >> r;
            // store user input in the global final_circles vector
            final_circles.push_back(vector<int>{x, y, r});
        }
    } else {
        file_in();  // read circle paramters from file
        if (option == 3) {
            glutIdleFunc(
                idle);  // if animation is required, register the idle function
        }
    }

    myinit();
    glutMainLoop();

    return 0;
}

// file input function
void file_in() {
    ifstream circleStream;  // create a filestream
    circleStream.clear();
    string filename;
    cout << "Enter file name: ";  // prompt user enter the filename
    cin >> filename;
    circleStream.open(filename);

    // check if the file is succesfully opened
    if (!circleStream) {
        cerr << "failed to open " << filename;
        exit(1);
    }

    int num_circles;
    circleStream >> num_circles;
    int x, y, r;
    int max_abs_coord = 0;
    int curr_max_abs_coord;

    // record the maximum absolute coorinate
    for (int i = 0; i < num_circles; ++i) {
        circleStream >> x >> y >> r;
        scaled_circles.push_back(vector<int>{x, y, r});
        curr_max_abs_coord =
            max(max(abs(x + r), abs(x - r)), max(abs(y + r), abs(y - r)));
        if (curr_max_abs_coord > max_abs_coord) {
            max_abs_coord = curr_max_abs_coord;
        }
    }
    // if the maximum absolute coorinate is too large, scale down circles to fit
    // them in the window
    if (2 * max_abs_coord > WINDOW_WIDTH) {
        adjustScale(scaled_circles, WINDOW_WIDTH, 2 * max_abs_coord);
    }
    final_circles = scaled_circles;
    toScreenCoordinate(final_circles);
    circleStream.close();
}

// display(): This function is called once for _every_ frame.
void display(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glColor3f(1.0, 0.84, 0);  // draw in golden yellow
    glPointSize(1.0);         // size of each point

    // draw circles stored in final_circles
    for (const vector<int>& circle : final_circles) {
        drawCircle(circle[0], circle[1], circle[2]);
    }

    glFlush();  // render graphics

    glutSwapBuffers();  // swap buffers
}

// myinit(): Set up attributes and viewing
void myinit() {
    glClearColor(0.0, 0.0, 0.0, 0.0);  // black background

    // set up viewing
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
}

void circlePoint(int x, int y, int center_x, int center_y) {
    // draw eight symmertic points at once
    glBegin(GL_POINTS);

    glVertex2i(x, y);
    glVertex2i(center_x - center_y + y, center_y - center_x + x);

    glVertex2i(2 * center_x - x, y);
    glVertex2i(center_x + center_y - y, center_y - center_x + x);

    glVertex2i(x, 2 * center_y - y);
    glVertex2i(center_x - center_y + y, center_y + center_x - x);

    glVertex2i(2 * center_x - x, 2 * center_y - y);
    glVertex2i(center_x + center_y - y, center_y + center_x - x);

    glEnd();
}

// implement Bresenham's algorithm described in part (a)
void drawCircle(int x, int y, int r) {
    circlePoint(x, y + r, x, y);
    int D = 5 - 4 * r;
    int curr_x = r;
    int curr_y = 0;
    while (curr_x >= curr_y) {
        curr_y += 1;
        if (D >= 0) {
            curr_x -= 1;
            D += 8 * curr_y - 8 * curr_x + 4;
        } else {
            D += 8 * curr_y + 4;
        }
        circlePoint(x + curr_x, y + curr_y, x, y);
    }
}

// adjust the x, y, r for every circle int the vector
void adjustScale(vector<vector<int>>& circles, int numerator, int denominator) {
    for (vector<int>& circle : circles) {
        for (size_t i = 0; i < circle.size(); ++i) {
            circle[i] = circle[i] * numerator / denominator;
        }
    }
}

// adjust r for every circle int the vector, keep x, y unchanged
void adjustSize(vector<vector<int>>& circles, int numerator, int denominator) {
    for (vector<int>& circle : circles) {
        circle[2] = circle[2] * numerator / denominator;
    }
}

// increse the x and y-coordinate to transform to screen coordinate
void toScreenCoordinate(vector<vector<int>>& circles) {
    for (vector<int>& circle : circles) {
        circle[0] += WINDOW_WIDTH / 2;
        circle[1] += WINDOW_HEIGHT / 2;
    }
}

void idle(void) {
    final_circles = scaled_circles;
    // scale down the already scaled circles again by factor of curr_frame/FRAME
    adjustSize(final_circles, curr_frame, FRAME);
    toScreenCoordinate(final_circles);
    ++curr_frame;  // every frame increase the scale factor by 1/FRAME
    if (curr_frame > FRAME) curr_frame = 1;
    glutPostRedisplay();  // or call display()
}

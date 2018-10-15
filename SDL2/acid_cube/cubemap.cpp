#include<cstdio>
#include<cstdlib>
#include<ctime>
#include<unistd.h>
#include "MX_interface.h"
#include<acidcam/ac.h>
#define WIDTH 1280
#define HEIGHT 720
#include<opencv2/opencv.hpp>

GLuint textures[10];
GLuint background_texture;
GLuint logo_texture;

float spin_x = 0, spin_y = 0, spin_z = 0;
float dist = -3.0;
int axis = 0;
bool start = true, going = false;
bool start_wait = false;

void updateTexture(cv::Mat &frame, GLuint &tex);
void genTextureFromFrame(cv::Mat &frame, GLuint &tex);

std::string filename;

namespace MX_i {
    int FPS = 30;
    float dt = 0;
}

using MX_i::FPS;
using MX_i::dt;
int old_t = 0;


GLfloat vertices[] = { -1.0f, -1.0f, 1.0f, // front face
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    
    -1.0f, -1.0f, -1.0f, // left side
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, -1.0f,
    
    -1.0f, 1.0f, -1.0f,
    -1.0f, -1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    
    -1.0f, 1.0f, -1.0f, // top
    -1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    
    1.0f, 1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    
    -1.0f, -1.0f, -1.0f, // bottom
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    
    1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    
    1.0f, -1.0f, -1.0f, // right
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, -1.0f,
    
    1.0f, 1.0f, -1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f,
    
    -1.0f, -1.0f, -1.0f, // back face
    1.0f, 1.0f, -1.0f,
    -1.0f, 1.0f, -1.0f,
    
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, 1.0f, -1.0f,
    
};

GLfloat texCoords[] = {
    0, 0, // front
    1, 1,
    0, 1,
    
    0, 0,
    1, 0,
    1, 1,
    
    0, 0, // left
    1, 0,
    0, 1,
    
    0, 1,
    1, 0,
    1, 1,
    
    0,0, // top
    0,1,
    1,1,
    
    1, 1,
    1, 0,
    0, 0,
    
    0, 0,// bottom
    0, 1,
    1, 1,
    
    1,1,
    1,0,
    0,0,
    
    0,0,// right
    1,0,
    0,1,
    
    0,1,
    1,0,
    1,1,
    
    0,0, // back
    1,1,
    0,1,
    
    0,0,
    1,0,
    1,1
};


GLfloat rot[4] = {0,-10,0,0};//{ -25, -20, 0, 0};
int width=WIDTH, height=HEIGHT;


cv::VideoCapture cap;
static int filter_index = 0;

unsigned int timer_callback(unsigned int t) {
    
    if(MX_i::PollController(MX_i::B_LEFT)) {
        if(filter_index > 0)
            --filter_index;
    }
    else if(MX_i::PollController(MX_i::B_RIGHT)) {
        if(filter_index < ac::draw_max-6)
            ++filter_index;
    }
    
    if(MX_i::PollController(MX_i::B_UP)) {
        if(axis > 0)
            --axis;
    }
    else if(MX_i::PollController(MX_i::B_DOWN)) {
        if(axis < 3)
            ++axis;
    }
    
    if(MX_i::PollController(MX_i::B_1)) {
        start_wait = true;
    }
    
    return t;
}

void idle() {
    
    if(start == true) {
        spin_x += 16.0f;
        dist -= 0.1f;
        if(dist <= -20.0f) {
            going = true;
            start = false;
        }
    }
    else if(going == true) {
        dist += 0.01f;
        spin_x -= 16.0f;
        if(dist > 1.0f) {
            going = false;
            start = true;
        }
    }
    
    int t = MX_i::GetTicks();
    int passed = t - old_t;
    old_t = t ;
    dt = passed;
    MX_i::Redisplay();
}


GLfloat frontFace[] = {
    -1.0f, -1.0f, 1.0f, // front face
    1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f,
    1.0f, -1.0f, 1.0f,
    1.0f, 1.0f, 1.0f
};

GLfloat frontTexture[] = {
    0, 0, // front
    1, 1,
    0, 1,
    
    0, 0,
    1, 0,
    1, 1,
};


static float intro_zPos = 0, intro_yRot = 0;

namespace cv {
    void applyColorMap(cv::_InputArray const&, cv::_OutputArray const&, int) {}
}



void renderIntro() {
    
    if(start_wait == false) return;
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    //gluOrtho2D(0, 640, 0, 480);
    gluOrtho2D(-1.0f, 1.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    /*
     cv::Mat frame;
     bool r = cap.read(frame);
     if(r == false) {
     cap.set(CV_CAP_PROP_POS_FRAMES, 1);
     cap.read(frame);
     }
     cv::Mat outframe;
     cv::flip(frame,outframe, 0);
     frame = outframe.clone();
     */
    
    cv::Mat frame;
    bool frame_read = cap.read(frame);
    if(frame_read == false) {
        cap.set(CV_CAP_PROP_POS_FRAMES,1);
        frame_read = cap.read(frame);
        std::cout << "video reset...\n";
    }
    
    if(frame_read) {
        cv::Mat outframe;
        cv::flip(frame,outframe, 0);
        frame = outframe.clone();
        ac::draw_func[filter_index](frame);
        updateTexture(frame, logo_texture);
        
        /*
         glBindTexture(GL_TEXTURE_2D, logo_texture);
         glEnableClientState(GL_VERTEX_ARRAY);
         glEnableClientState(GL_TEXTURE_COORD_ARRAY);
         glVertexPointer(3, GL_FLOAT, 0, frontFace);
         glTexCoordPointer(2, GL_FLOAT, 0, frontTexture);
         glDrawArrays(GL_TRIANGLES, 0, 6);
         glDisableClientState(GL_VERTEX_ARRAY);
         glDisableClientState(GL_TEXTURE_COORD_ARRAY);
         */
        
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0f, (float)(width/height), 0.1f, 300.0f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        
        glTranslatef(0.0f, 0.0f, dist);
        switch(axis) {
            case 0:
                glRotatef(spin_x, 0.0f, 0.0f, 1.0f);
                break;
            case 1:
                glRotatef(spin_x, 1.0f, 0.0f, 0.0f);
                break;
            case 2:
                glRotatef(spin_x, 1.0f, 1.0f, 0.0f);
                break;
            case 3:
                glRotatef(spin_x, 1.0f, 1.0f, 1.0f);
                break;
        }
        
        /*
         glTranslatef(0, 0, -4.0);
         
         glRotatef(intro_yRot, 0, 1, 0);
         intro_yRot += 0.1f * dt;
         if(intro_yRot > 360) {
         intro_yRot = 1;
         } else if(intro_yRot > 180) {
         static float xRot = 0.0f;
         xRot += 0.1f * dt;
         glRotatef(xRot, 1, 0, 0);
         }*/
        
        glVertexPointer(3, GL_FLOAT, 0, vertices);
        glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
        glBindTexture(GL_TEXTURE_2D, logo_texture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //	glLoadIdentity();
    glPushMatrix();
    renderIntro();
    glPopMatrix();
    MX_i::SwapBuffers();
}

void resize(int w, int h) {
    if(w <= 0 || h <= 0) w = 1, h = 1;
    width = w, height = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, float(w / h), 0.1f, 300.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void genTextureFromFrame(cv::Mat &frame, GLuint &tex) {
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frame.cols, frame.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, frame.ptr());
}

void updateTexture(cv::Mat &frame, GLuint &tex) {
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame.cols, frame.rows, GL_BGR, GL_UNSIGNED_BYTE, frame.ptr());
}

void  init() {
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    resize(width, height);
    srand((unsigned int)time(0));
    glEnable(GL_TEXTURE_2D);
    cv::Mat frame;
    if(cap.read(frame)) {
        genTextureFromFrame(frame, logo_texture);
    }
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    MX_i::CreateTimer(timer_callback, 150);
}

void clean() {
    
}

int main(int argc, char **argv) {
    int cx = 0;
    int cy = 0;
    if(argc == 4 || argc == 5) {
        filename = argv[1];
        cap.open(filename);
        if(!cap.isOpened()) {
            std::cerr << "Error could not load video: " << filename << " ...\n";
            exit(EXIT_FAILURE);
        }
        cx = atoi(argv[2]);
        cy = atoi(argv[3]);
        if(cx <= 0 || cy <= 0 ) {
            std::cerr << "Error must pass valid resolution\n";
            return -1;
        }
        if(argc == 5)
            filter_index = atoi(argv[4]);
    }
    else {
        std::cerr << "Requires:\nacid_cube filename width height\n";
        exit(EXIT_FAILURE);
    }
    
    MX_i::Init(&argc, argv, cx, cy);
    MX_i::SetCallbacks(render, idle, resize);
    init();
    MX_i::Loop();
    clean();
    MX_i::Quit();
    return 0;
}

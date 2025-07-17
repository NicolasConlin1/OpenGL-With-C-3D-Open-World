//Nicolas Conlin 
//CSCI 4229-001B

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#ifdef USEGLEW
#include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
//Default resolution
//For Retina displays compile with -DRES=2
#ifndef RES
#define RES 1
#endif
//th & zh are set to 0 & 50 for a visually better starting angle
int th=0;
int zh=50;

//Field of View
int fov=55;
//Aspect Ratio
double asp=1; 
//World Size
double dim=3.5;

//Variable to track the projection mode
int mode = 0;
//Variables to track light mode and position
int direction = 0;
double lightx = 0;
double lightz = 0;
double lighty = 3;
int ph = 90;
int qh = 90;

//Variables to keep track of aspects of the light
int light     =   1;  //Lighting
int inc       =  10;  //Ball increment
int smooth    =   1;  //Smooth/Flat shading
int local     =   0;  //Local Viewer Model
int emission  =   0;  //Emission intensity (%)
int ambient   =  10;  //Ambient intensity (%)
int diffuse   =  60;  //Diffuse intensity (%)
int specular  =   0;  //Specular intensity (%)
int shininess =   0;  //Shininess (power of two)
float shiny   =   1;  //Shininess (value)

//Coordinated for where the camera's looking
float lookx = 0.0;
float lookz = 5.0;
//Coordinates for the camera position
float camx = 0.0;
float camy = 0.5;
float camz = 0.0;
//camera rotation
int cRot = 0;
float dt = 0.05;

//variables to keep track of the cars in timer
double ch = 0;
double ch2 = 0;
int cSwitch = 0;
int cSwitch2 = 0;

//Arrays to keep track of the rng placement of grass & trees & RNG tree scaling
int RNG[441];
double RNG2[441];

//array of textures to be filled in Main
int textures[10];
//variable used to keep track of the texture mode
int texMode = 0;

#define Cos(th) cos(3.14159265/180*(th))
#define Sin(th) sin(3.14159265/180*(th))

//Basic print function used to label the axis'
#define LEN 8192 
void Print(const char* format , ...)
{
    char buf[LEN]; 
    char* ch = buf;
    va_list args;
    va_start(args,format);
    vsnprintf(buf,LEN,format,args);
    va_end(args);
    while (*ch)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*ch++);
    }
}

//Fatal function for the ifdef in main
//reused from canvas code
void Fatal(const char* format , ...)
{
   va_list args;
   va_start(args,format);
   vfprintf(stderr,format,args);
   va_end(args);
   exit(1);
}

//Function to load the texture from BMP file
//Reused from hw 6/canvas code
unsigned int LoadTexBMP(const char* file)
{
    unsigned int texture;      //Texture name
    FILE *f;                   //File pointer
    unsigned short magic;      //Image magic
    unsigned int dx, dy, size; //Image dimensions
    unsigned short nbp, bpp;   //Planes and bits per pixel
    unsigned char *image;      //Image data
    unsigned int off;          //Image offset
    unsigned int k;            //Counter
    unsigned int max;          //Maximum texture dimensions

    //Open file
    f = fopen(file, "rb");
    if (!f)
    {
        Fatal("Cannot open file %s\n", file);
    }
    //Check image magic
    if (fread(&magic, 2, 1, f) != 1)
    {
        Fatal("Cannot read magic from %s\n", file);
    }
    if (magic != 0x4D42 && magic != 0x424D)
    {
        Fatal("Image magic not BMP in %s\n", file);
    }    
    //Read header
    if (fseek(f, 8, SEEK_CUR) || fread(&off, 4, 1, f) != 1 || fseek(f, 4, SEEK_CUR) || fread(&dx, 4, 1, f) != 1 || fread(&dy, 4, 1, f) != 1 || fread(&nbp, 2, 1, f) != 1 || fread(&bpp, 2, 1, f) != 1 || fread(&k, 4, 1, f) != 1)
    {
        Fatal("Cannot read header from %s\n", file);
    }
    //Check image parameters
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, (int *)&max);
    if (dx < 1 || dx > max)
    {
        Fatal("%s image width %d out of range 1-%d\n", file, dx, max);
    }
    if (dy < 1 || dy > max)
    {
        Fatal("%s image height %d out of range 1-%d\n", file, dy, max);
    }
    if (nbp != 1)
    {
        Fatal("%s bit planes is not 1: %d\n", file, nbp);
    }
    if (bpp != 24)
    {
        Fatal("%s bits per pixel is not 24: %d\n", file, bpp);
    }
    if (k != 0)
    {
        Fatal("%s compressed files not supported\n", file);
    }
    #ifndef GL_VERSION_2_0
        //OpenGL 2.0 lifts the restriction that texture size must be a power of two
        for (k = 1; k < dx; k *= 2)
        {
            if (k != dx)
            {
                Fatal("%s image width not a power of two: %d\n", file, dx);
            }
        }
        for (k = 1; k < dy; k *= 2)
        {
            if (k != dy)
            {
                Fatal("%s image height not a power of two: %d\n", file, dy);
            }
        }   
    #endif
    //Allocate image memory
    size = 3*dx*dy;
    image = (unsigned char *)malloc(size);
    if (!image)
    {
        Fatal("Cannot allocate %d bytes of memory for image %s\n", size, file);
    }
    //Seek to and read image
    if (fseek(f, off, SEEK_SET) || fread(image, size, 1, f) != 1)
    {
        Fatal("Error reading data from image %s\n", file);
    }
    fclose(f);
    //Reverse colors (BGR -> RGB)
    for (k = 0; k < size; k += 3)
    {
        unsigned char temp = image[k];
        image[k] = image[k + 2];
        image[k + 2] = temp;
    }
    //Generate 2D texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    //Copy image
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, dx, dy, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    if (glGetError())
    {
        Fatal("Error in glTexImage2D %s %dx%d\n", file, dx, dy);
    }
    //Scale linearly when image size doesn't match
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //Free image memory
    free(image);
    //Return texture name
    return texture;
}

//Project function reused from hw5
//Unlike hw5 FP is now working properly
static void Project()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //Orthogonal
    if (mode == 2)
    {
        glOrtho(-asp*dim,+asp*dim, -dim,+dim, -dim,+dim);
    }
    //Perspective projection & FP
    else
    {
        gluPerspective(fov,asp,dim/4,4*dim);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

//A function to make the main trees for grassland
//xi & zi are coordinated from grassLand, while y is a randomized height scaler & rot is a randomized rotation value (also from grassLand)
static void tree(int xi, int zi, double yi, int rot)
{
    float white[] = {1,1,1,1};
    float black[] = {0,0,0,0};
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
    glPushMatrix();
    glScaled(1,yi,1);
    glRotated(rot,0,1,0);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,texMode?GL_REPLACE:GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D,textures[3]);
    //Roots
    glBegin(GL_TRIANGLES);
    glColor3f(5,2.5,0);
    glNormal3f(0.5,1.5,0.5);
    glTexCoord2f(0,0); glVertex3f(xi+2,0.01,zi);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,0.5,zi);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,0.01,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0.5,1.5,-0.5);
    glTexCoord2f(0,0); glVertex3f(xi+2,0.01,zi);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,0.5,zi);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,0.01,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-0.5,1.5,0.5);
    glTexCoord2f(0,0); glVertex3f(xi-2,0.01,zi);
    glTexCoord2f(1,0); glVertex3f(xi-0.5,0.5,zi);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,0.01,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-0.5,1.5,-0.5);
    glTexCoord2f(0,0); glVertex3f(xi-2,0.01,zi);
    glTexCoord2f(1,0); glVertex3f(xi-0.5,0.5,zi);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,0.01,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0.5,1.5,0.5);
    glTexCoord2f(0,0); glVertex3f(xi,0.01,zi+2);
    glTexCoord2f(1,0); glVertex3f(xi,0.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,0.01,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-0.5,1.5,0.5);
    glTexCoord2f(0,0); glVertex3f(xi,0.01,zi+2);
    glTexCoord2f(1,0); glVertex3f(xi,0.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,0.01,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0.5,1.5,-0.5);
    glTexCoord2f(0,0); glVertex3f(xi,0.01,zi-2);
    glTexCoord2f(1,0); glVertex3f(xi,0.5,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,0.01,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-0.5,1.5,-0.5);
    glTexCoord2f(0,0); glVertex3f(xi,0.01,zi-2);
    glTexCoord2f(1,0); glVertex3f(xi,0.5,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,0.01,zi-0.5);
    glEnd();
    //Base
    glBegin(GL_QUADS);
    glColor3f(5,2.5,0);
    glNormal3f(0.6,0,0);
    glTexCoord2f(0,0); glVertex3f(xi+0.5,0.01,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,0.01,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,1.5,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi+0.5,1.5,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,0.6);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,0.01,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,0.01,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,1.5,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi-0.5,1.5,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-0.6,0,0);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,0.01,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi-0.5,0.01,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,1.5,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi-0.5,1.5,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,-0.6);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,0.01,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,0.01,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,1.5,zi-0.5);
    glTexCoord2f(1,1); glVertex3f(xi-0.5,1.5,zi-0.5);
    //The body is split off into multiple sections in order to curve it in unusual ways
    //The same is done for the branches
    //Body 1
    glColor3f(5,2.5,0);
    glNormal3f(1,-0.5,0);
    glTexCoord2f(0,0); glVertex3f(xi+0.5,1.5,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,1.5,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi+1,2.5,zi-0.5);
    glTexCoord2f(1,1); glVertex3f(xi+1,2.5,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,0.6);
    glTexCoord2f(0,0); glVertex3f(xi+0.5,1.5,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi-0.5,1.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi,2.5,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi+1,2.5,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-1,0.5,0);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,1.5,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi-0.5,1.5,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi,2.5,zi-0.5);
    glTexCoord2f(1,1); glVertex3f(xi,2.5,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,-0.6);
    glTexCoord2f(0,0); glVertex3f(xi+0.5,1.5,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi-0.5,1.5,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi,2.5,zi-0.5);
    glTexCoord2f(1,1); glVertex3f(xi+1,2.5,zi-0.5);
    //Body 2
    glColor3f(5,2.5,0);
    glNormal3f(1.5,2,0);
    glTexCoord2f(0,0); glVertex3f(xi+1,2.5,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi+1,2.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi-1,4,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi-1,4,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,0.6);
    glTexCoord2f(0,0); glVertex3f(xi,2.5,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi+1,2.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi-1,4,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi-2,4,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-1.5,-2,0);
    glTexCoord2f(0,0); glVertex3f(xi,2.5,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi,2.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi-2,4,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi-2,4,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,-0.6);
    glTexCoord2f(0,0); glVertex3f(xi,2.5,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi+1,2.5,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi-1,4,zi-0.5);
    glTexCoord2f(1,1); glVertex3f(xi-2,4,zi-0.5);
    //Body 3
    glColor3f(5,2.5,0);
    glNormal3f(1.6,-1.6,0);
    glTexCoord2f(0,0); glVertex3f(xi-1,4,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi-1,4,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,5.5,zi-0.5);
    glTexCoord2f(1,1); glVertex3f(xi+0.5,5.5,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,0.6);
    glTexCoord2f(0,0); glVertex3f(xi-1,4,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi-2,4,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,5.5,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi+0.5,5.5,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-1.6,1.6,0);
    glTexCoord2f(0,0); glVertex3f(xi-2,4,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi-2,4,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,5.5,zi-0.5);
    glTexCoord2f(1,1); glVertex3f(xi-0.5,5.5,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,-0.6);
    glTexCoord2f(0,0); glVertex3f(xi-1,4,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi-2,4,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,5.5,zi-0.5);
    glTexCoord2f(1,1); glVertex3f(xi+0.5,5.5,zi-0.5);
    //Body final
    glColor3f(5,2.5,0);
    glNormal3f(0.6,0,0);
    glTexCoord2f(0,0); glVertex3f(xi+0.5,5.5,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,5.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,7.5,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi+0.5,7.5,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,0.6);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,5.5,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,5.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,8,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi-0.5,8,zi+0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-0.6,0,0);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,5.5,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi-0.5,5.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,7.5,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi-0.5,7.5,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,-0.6);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,5.5,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,5.5,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,7.5,zi-0.5);
    glTexCoord2f(1,1); glVertex3f(xi-0.5,7.5,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(0,1,-0.5);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,7.5,zi-0.5);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,7.5,zi-0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,8,zi+0.5);
    glTexCoord2f(1,1); glVertex3f(xi-0.5,8,zi+0.5);
    glEnd();
    glBegin(GL_TRIANGLES);
    glColor3f(5,2.5,0);
    glNormal3f(0.6,0,0);
    glTexCoord2f(0,0); glVertex3f(xi+0.5,8,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi+0.5,7.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi+0.5,7.5,zi-0.5);
    glColor3f(5,2.5,0);
    glNormal3f(-0.6,0,0);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,8,zi+0.5);
    glTexCoord2f(1,0); glVertex3f(xi-0.5,7.5,zi+0.5);
    glTexCoord2f(0,1); glVertex3f(xi-0.5,7.5,zi-0.5);
    glEnd();
    //Branches
    //B-1
    glBegin(GL_QUADS);
    //A simple black square is placed at the base of each branch that's directly connected to the tree
    //It's not scene so it's now commented out, but it made finding the starting position much easier.
    //All commented chuncks of code in tree are these squares
    /*glColor3f(1,1,1);
    glTexCoord2f(0,0); glVertex3f(xi+0.25,3.0624,zi-0.25);
    glTexCoord2f(1,0); glVertex3f(xi+0.25,3.0625,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi-0.25,3.4375,zi+0.25);
    glTexCoord2f(1,1); glVertex3f(xi-0.25,3.4375,zi-0.25);*/
    glColor3f(5,2.5,0);
    glNormal3f(0,0,0.26);
    glTexCoord2f(0,0); glVertex3f(xi+0.25,3.0625,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi-0.25,3.4375,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+1,4.5,zi+0.25);
    glTexCoord2f(1,1); glVertex3f(xi+1.25,4,zi+0.25);
    glColor3f(5,2.5,0);
    glNormal3f(0.9375,-1,0);
    glTexCoord2f(0,0); glVertex3f(xi+0.25,3.0624,zi-0.25);
    glTexCoord2f(1,0); glVertex3f(xi+0.25,3.0625,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+1.25,4,zi+0.25);
    glTexCoord2f(1,1); glVertex3f(xi+1.25,4,zi-0.25);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,-0.26);
    glTexCoord2f(0,0); glVertex3f(xi+0.25,3.0625,zi-0.25);
    glTexCoord2f(1,0); glVertex3f(xi-0.25,3.4375,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi+1,4.5,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi+1.25,4,zi-0.25);
    glColor3f(5,2.5,0);
    glNormal3f(-1.0625,1.25,0);
    glTexCoord2f(0,0); glVertex3f(xi-0.25,3.4375,zi-0.25);
    glTexCoord2f(1,0); glVertex3f(xi-0.25,3.4375,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+1,4.5,zi+0.25);
    glTexCoord2f(1,1); glVertex3f(xi+1,4.5,zi-0.25);
    //B-1-1
    glColor3f(5,2.5,0);
    glNormal3f(0,0,0.26);
    glTexCoord2f(0,0); glVertex3f(xi+1,4.5,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi+1.25,4,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+2.25,4.25,zi+0.25);
    glTexCoord2f(1,1); glVertex3f(xi+2,4.75,zi+0.25);
    glColor3f(5,2.5,0);
    glNormal3f(0.25,-1,0);
    glTexCoord2f(0,0); glVertex3f(xi+1.25,4,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi+1.25,4,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi+2.25,4.25,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi+2.25,4.25,zi+0.25);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,-0.26);
    glTexCoord2f(0,0); glVertex3f(xi+1,4.5,zi-0.25);
    glTexCoord2f(1,0); glVertex3f(xi+1.25,4,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi+2.25,4.25,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi+2,4.75,zi-0.25);
    glColor3f(5,2.5,0);
    glNormal3f(-0.25,1,0);
    glTexCoord2f(0,0); glVertex3f(xi+1,4.5,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi+1,4.5,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi+2,4.75,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi+2,4.75,zi+0.25);
    glColor3f(5,2.5,0);
    glNormal3f(0.5,0.25,0);
    glTexCoord2f(0,0); glVertex3f(xi+2,4.75,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi+2.25,4.25,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+2.25,4.25,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi+2,4.75,zi-0.25);
    glEnd();
    //B-1-1-1
    glBegin(GL_TRIANGLES);
    //The same black starting spot is used for smaller branches
    /*glColor3f(1,1,1);
    //2 of the points were raised up by .25 then given an angle by adding the y-increase the part of the main branch it rested on had rised at that point
    glTexCoord2f(0,0); glVertex3f(xi+1.25,4,zi+0.26);
    glTexCoord2f(1,0); glVertex3f(xi+1.5,(4.25+0.0625),zi+0.26);
    glTexCoord2f(0,1); glVertex3f(xi+1,(4.25-0.234375),zi+0.26);*/
    glColor3f(5,2.5,0);
    glNormal3f(2.25,-0.3125,1.5);
    glTexCoord2f(0,0); glVertex3f(xi+1.25,4,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi+1.5,(4.25+0.0625),zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+1.25,4.2,zi+2.5);
    glColor3f(5,2.5,0);
    glNormal3f(-0.3125,-2.25,1.25);
    glTexCoord2f(0,0); glVertex3f(xi+1.25,4,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi+1,(4.25-0.234375),zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+1.25,4.2,zi+2.5);
    glColor3f(5,2.5,0);
    glNormal3f(-1.25,2.25,0.3125);
    glTexCoord2f(0,0); glVertex3f(xi+1.5,(4.25+0.0625),zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi+1,(4.25-0.234375),zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+1.25,4.2,zi+2.5);
    //B-1-1-2
    /*glColor3f(1,1,1);
    glTexCoord2f(0,0); glVertex3f(xi+2,4.76,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi+2,4.76,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi+1.75,(4.76-0.0625),zi);*/
    glColor3f(5,2.5,0);
    glNormal3f(2,0,0);
    glTexCoord2f(0,0); glVertex3f(xi+2,4.75,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi+2,4.75,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi+2,6,zi);
    glColor3f(5,2.5,0);
    glNormal3f(-1.25,0.25,-0.25);
    glTexCoord2f(0,0); glVertex3f(xi+1.75,(4.75-0.0625),zi);
    glTexCoord2f(1,0); glVertex3f(xi+2,4.75,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi+2,6,zi);
    glColor3f(5,2.5,0);
    glNormal3f(-1.25,0.25,0.25);
    glTexCoord2f(0,0); glVertex3f(xi+1.75,(4.75-0.0625),zi);
    glTexCoord2f(1,0); glVertex3f(xi+2,4.75,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+2,6,zi);
    //B-0-0-1
    /*glEnd();
    glBegin(GL_QUADS);
    glColor3f(1,1,1);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,2.874,zi);
    glTexCoord2f(1,0); glVertex3f(xi-1,3.24,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi-1.5,3.624,zi);
    glTexCoord2f(1,1); glVertex3f(xi-1,3.24,zi-0.25);
    glEnd();*/
    glColor3f(5,2.5,0);
    glNormal3f(1,-0.5,1.5);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,2.874,zi);
    glTexCoord2f(1,0); glVertex3f(xi-1,3.24,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi-2,2.24,zi);
    glColor3f(5,2.5,0);
    glNormal3f(-1,0.5,-1.5);
    glTexCoord2f(0,0); glVertex3f(xi-1.5,3.624,zi);
    glTexCoord2f(1,0); glVertex3f(xi-1,3.24,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi-2,2.24,zi);
    glColor3f(5,2.5,0);
    glNormal3f(-1,0.5,1.5);
    glTexCoord2f(0,0); glVertex3f(xi-1,3.24,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi-1.5,3.624,zi);
    glTexCoord2f(0,1); glVertex3f(xi-2,2.24,zi);
    glColor3f(5,2.5,0);
    glNormal3f(1,-0.5,-1.5);
    glTexCoord2f(0,0); glVertex3f(xi-0.5,2.874,zi);
    glTexCoord2f(1,0); glVertex3f(xi-1,3.24,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi-2,2.24,zi);
    glEnd();
    //B-2
    glBegin(GL_QUADS);
    /*glColor3f(1,1,1);
    glTexCoord2f(0,0); glVertex3f(xi-1.25,4.76,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi-1.25,4.76,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi-0.875,5.126,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi-0.875,5.126,zi+0.25);*/
    glColor3f(5,2.5,0);
    glNormal3f(-0.5,-0.75,0);
    glTexCoord2f(0,0); glVertex3f(xi-1.25,4.75,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi-1.25,4.75,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi-2,5.25,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi-2,5.25,zi+0.25);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,0.26);
    glTexCoord2f(0,0); glVertex3f(xi-1.25,4.75,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi-0.875,5.125,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi-1.625,5.625,zi+0.25);
    glTexCoord2f(1,1); glVertex3f(xi-2,5.25,zi+0.25);
    glColor3f(5,2.5,0);
    glNormal3f(0,0,-0.26);
    glTexCoord2f(0,0); glVertex3f(xi-1.25,4.75,zi-0.25);
    glTexCoord2f(1,0); glVertex3f(xi-0.875,5.125,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi-1.625,5.625,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi-2,5.25,zi-0.25);
    glColor3f(5,2.5,0);
    glNormal3f(0.5,0.75,0);
    glTexCoord2f(0,0); glVertex3f(xi-0.875,5.125,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi-0.875,5.125,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi-1.625,5.625,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi-1.625,5.625,zi+0.25);
    glEnd();
    glBegin(GL_TRIANGLES);
    glColor3f(5,2.5,0);
    glNormal3f(-0.25,-0.25,0);
    glTexCoord2f(0,0); glVertex3f(xi-2,5.25,zi-0.25);
    glTexCoord2f(1,0); glVertex3f(xi-2,5.25,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi-2.25,5.5,zi);
    glColor3f(5,2.5,0);
    glNormal3f(+0.25,0.25,0);
    glTexCoord2f(0,0); glVertex3f(xi-1.625,5.625,zi-0.25);
    glTexCoord2f(1,0); glVertex3f(xi-1.625,5.625,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi-1.875,5.875,zi);
    glEnd();
    //B-2-1
    glBegin(GL_QUADS);
    /*glColor3f(1,1,1);
    glTexCoord2f(0,0); glVertex3f(xi-1.875,5.875,zi);
    glTexCoord2f(1,0); glVertex3f(xi-2.25,5.5,zi);
    glTexCoord2f(0,1); glVertex3f(xi-2,5.25,zi+0.25);
    glTexCoord2f(1,1); glVertex3f(xi-1.625,5.625,zi+0.25);*/
    glColor3f(5,2.5,0);
    glNormal3f(-0.75,0.6,-0.5);
    glTexCoord2f(0,0); glVertex3f(xi-1.875,5.875,zi);
    glTexCoord2f(1,0); glVertex3f(xi-2.25,5.5,zi);
    glTexCoord2f(0,1); glVertex3f(xi-2.75,6,zi+0.75);
    glTexCoord2f(1,1); glVertex3f(xi-2.375,6.375,zi+0.75);
    glColor3f(5,2.5,0);
    glNormal3f(-0.75,-0.75,-0.25);
    glTexCoord2f(0,0); glVertex3f(xi-2.25,5.5,zi);
    glTexCoord2f(1,0); glVertex3f(xi-2,5.25,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi-2.5,5.75,zi+1);
    glTexCoord2f(1,1); glVertex3f(xi-2.75,6,zi+0.75);
    glColor3f(5,2.5,0);
    glNormal3f(0.75,-0.6,0.5);
    glTexCoord2f(0,0); glVertex3f(xi-2,5.25,zi+0.25);
    glTexCoord2f(1,0); glVertex3f(xi-1.625,5.625,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi-2.125,6.125,zi+1);
    glTexCoord2f(1,1); glVertex3f(xi-2.5,5.75,zi+1);
    glColor3f(5,2.5,0);
    glNormal3f(0.75,0.75,0.25);
    glTexCoord2f(0,0); glVertex3f(xi-1.875,5.875,zi);
    glTexCoord2f(1,0); glVertex3f(xi-1.625,5.625,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi-2.125,6.125,zi+1);
    glTexCoord2f(1,1); glVertex3f(xi-2.375,6.375,zi+0.75);
    glColor3f(5,2.5,0);
    glNormal3f(-0.25,+0.375,0.375);
    glTexCoord2f(0,0); glVertex3f(xi-2.75,6,zi+0.75);
    glTexCoord2f(1,0); glVertex3f(xi-2.375,6.375,zi+0.75);
    glTexCoord2f(0,1); glVertex3f(xi-2.125,6.125,zi+1);
    glTexCoord2f(1,1); glVertex3f(xi-2.5,5.75,zi+1);
    //B-2-2
    /*glColor3f(1,1,1);
    glTexCoord2f(0,0); glVertex3f(xi-1.875,5.875,zi);
    glTexCoord2f(1,0); glVertex3f(xi-2.25,5.5,zi);
    glTexCoord2f(0,1); glVertex3f(xi-2,5.25,zi-0.25);
    glTexCoord2f(1,1); glVertex3f(xi-1.625,5.625,zi-0.25);*/
    glColor3f(5,2.5,0);
    glNormal3f(-1.25,0.5,0.25);
    glTexCoord2f(0,0); glVertex3f(xi-1.875,5.875,zi);
    glTexCoord2f(1,0); glVertex3f(xi-2.25,5.5,zi);
    glTexCoord2f(0,1); glVertex3f(xi-2.5,6,zi-1.25);
    glTexCoord2f(1,1); glVertex3f(xi-2.125,6.375,zi-1.25);
    glColor3f(5,2.5,0);
    glNormal3f(-0.5,-1.5,-0.25);
    glTexCoord2f(0,0); glVertex3f(xi-2.25,5.5,zi);
    glTexCoord2f(1,0); glVertex3f(xi-2,5.25,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi-2.25,5.75,zi-1.5);
    glTexCoord2f(1,1); glVertex3f(xi-2.5,6,zi-1.25);
    glColor3f(5,2.5,0);
    glNormal3f(1.25,-0.5,-0.25);
    glTexCoord2f(0,0); glVertex3f(xi-2,5.25,zi-0.25);
    glTexCoord2f(1,0); glVertex3f(xi-1.625,5.625,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi-1.875,6.125,zi-1.5);
    glTexCoord2f(1,1); glVertex3f(xi-2.25,5.75,zi-1.5);
    glColor3f(5,2.5,0);
    glNormal3f(0.5,1.5,0.25);
    glTexCoord2f(0,0); glVertex3f(xi-1.875,5.875,zi);
    glTexCoord2f(1,0); glVertex3f(xi-1.625,5.625,zi-0.25);
    glTexCoord2f(0,1); glVertex3f(xi-1.875,6.125,zi-1.5);
    glTexCoord2f(1,1); glVertex3f(xi-2.125,6.375,zi-1.25);
    glColor3f(5,2.5,0);
    glNormal3f(-0.25,0.25,-0.625);
    glTexCoord2f(0,0); glVertex3f(xi-2.5,6,zi-1.25);
    glTexCoord2f(1,0); glVertex3f(xi-2.125,6.375,zi-1.25);
    glTexCoord2f(0,1); glVertex3f(xi-1.875,6.125,zi-1.5);
    glTexCoord2f(1,1); glVertex3f(xi-2.25,5.75,zi-1.5);
    glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

//A small function to make leaves for grassland
//xi and zi are the same as the description for tree, although the values for xi & zi are always different
static void leaf(int xi, int zi, int rot)
{
    float white[] = {1,1,1,1};
    float black[] = {0,0,0,0};
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
    glPushMatrix();
    glRotated(rot,0,1,0);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,texMode?GL_REPLACE:GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D,textures[3]);
    glBegin(GL_QUADS);
    glColor3f(1,1,0);
    glNormal3f(0,0.1,0);
//Many values are slightly higher/lower than objects they're on (i.e. the height of the leaf vs the grass plot)
//This is done to avoid clipping
    glTexCoord2f(0,0); glVertex3f(xi-0.25,-0.01,zi);
    glTexCoord2f(1,0); glVertex3f(xi,-0.01,zi+0.25);
    glTexCoord2f(0,1); glVertex3f(xi+0.75,-0.01,zi);
    glTexCoord2f(1,1); glVertex3f(xi,-0.01,zi-0.25);
    glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

//A patch of grass with tree's, leaves, and grass
//There are multiple types of grassLand, each with different RNG checkers to add additional randomization
static void grassLand(double x, double z, int type)
{
    float white[] = {1,1,1,1};
    float black[] = {0,0,0,0};
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
    glPushMatrix();
    glTranslated(x,-0.01,z);
    glScaled(0.1,0.1,0.1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,texMode?GL_REPLACE:GL_MODULATE);
    //Main tile
    glBindTexture(GL_TEXTURE_2D,textures[9]);
    glBegin(GL_QUADS);
    glColor3f(0,1,1);
    glNormal3f(0,0.1,0);
    glTexCoord2f(0,0); glVertex3f(-10,-0.02,10);
    glTexCoord2f(1,0); glVertex3f(10,-0.02,10);
    glTexCoord2f(0,1); glVertex3f(10,-0.02,-10);
    glTexCoord2f(1,1); glVertex3f(-10,-0.02,-10);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    //Random 3d grass & leaves
    //For every x and z coordinate there is chance either peice of grass or a leaf will be present
    //This is handled through rng to add a slight bit of visual variety between boots and to save time placing each peice. 
    if (type == 0)
    {
        int counter = 0;
        for (int xi = -10; xi <= 10; xi++)
        {
            for (int zi = -10; zi <= 10; zi++)
            {
                //Grass has a 30% chance
                if (RNG[counter] < 30)
                {
                    glBegin(GL_LINES);
                    glColor3f(0,1,0);
                    glVertex3d(xi,0,zi);
                    glVertex3d(xi,0.8,zi);
                    glEnd();
                }
                //Leaves have a 20% chance
                if (RNG[counter] < 99 && RNG[counter] > 78)
                {
                    leaf(xi,zi,RNG[counter]);
                }
                counter++;
            }
        }
        //RNG trees
        //Tree's are also rng to add more visual variety between boots
        counter = 0;
        for (int xi = -8; xi < 9; xi++)
        {
            for (int zi = -8; zi < 9; zi++)
            {
                //There's a 1% chance of a tree spawning
                if (RNG[counter] == 99)
                {
                    tree(xi,zi,RNG2[counter],RNG[counter+1]+RNG[counter+2]);
                }
                counter++;
            }
        }
    }
    //Different patches of grass have different checking values for RNG
    //This allows grass patches to be next to eachother without being identical
    //This also means theres a smaller chance an element won't spawn on any individual boot
    else if (type == 1)
    {
        int counter = 0;
        for (int xi = -10; xi <= 10; xi++)
        {
            for (int zi = -10; zi <= 10; zi++)
            {
                //Grass has a 30% chance
                if (RNG[counter] <= 99 && RNG[counter] > 69)
                {
                    glBegin(GL_LINES);
                    glColor3f(0,1,0);
                    glVertex3d(xi,0,zi);
                    glVertex3d(xi,0.8,zi);
                    glEnd();
                }
                //Leaves have a 20% chance
                if (RNG[counter] < 20)
                {
                    leaf(xi,zi,RNG[counter]);
                }
                counter++;
            }
        }
        //RNG trees
        //Tree's are also rng to add more visual variety between boots
        counter = 0;
        for (int xi = -8; xi < 9; xi++)
        {
            for (int zi = -8; zi < 9; zi++)
            {
                //There's a 1% chance of a tree spawning
                if (RNG[counter] == 50)
                {
                    tree(xi,zi,RNG2[counter],RNG[counter+1]+RNG[counter+2]);
                }
                counter++;
            }
        }
    }
    else if (type == 2)
    {
        int counter = 0;
        for (int xi = -10; xi <= 10; xi++)
        {
            for (int zi = -10; zi <= 10; zi++)
            {
                //Grass has a 30% chance
                if (RNG[counter] >= 30 && RNG[counter] < 60)
                {
                    glBegin(GL_LINES);
                    glColor3f(0,1,0);
                    glVertex3d(xi,0,zi);
                    glVertex3d(xi,0.8,zi);
                    glEnd();
                }
                //Leaves have a 20% chance
                if (RNG[counter] >= 60 && RNG[counter] <80)
                {
                    leaf(xi,zi,RNG[counter]);
                }
                counter++;
            }
        }
        //RNG trees
        //Tree's are also rng to add more visual variety between boots
        counter = 0;
        for (int xi = -8; xi < 9; xi++)
        {
            for (int zi = -8; zi < 9; zi++)
            {
                //There's a 1% chance of a tree spawning
                if (RNG[counter] == 81)
                {
                    tree(xi,zi,RNG2[counter],RNG[counter+1]+RNG[counter+2]);
                }
                counter++;
            }
        }
    }
    //The final mode only has grass
    //This is useful for plots with a structure on it (pine tree) or when trees would make it feel cluttered
    else
    {
        int counter = 0;
        for (int xi = -10; xi <= 10; xi++)
        {
            for (int zi = -10; zi <= 10; zi++)
            {
                //Grass has a 30% chance
                if (RNG[counter] < 10 || RNG[counter] >= 80)
                {
                    glBegin(GL_LINES);
                    glColor3f(0,1,0);
                    glVertex3d(xi,0,zi);
                    glVertex3d(xi,0.8,zi);
                    glEnd();
                }
                counter++;
            }
        }}
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

//A large pine tree meant to the centerpiece for the park
static void pineTree(double x, double z)
{
    float white[] = {1,1,1,1};
    float black[] = {0,0,0,0};
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
    glPushMatrix();
    glTranslated(x,-0.01,z);
    glScaled(0.1,0.1,0.1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,texMode?GL_REPLACE:GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D,textures[6]);
    //Roots
    glBegin(GL_TRIANGLES);
    glColor3f(1,1,1);
    glNormal3f(-5,3.5,2.5);
    glTexCoord2f(0,0); glVertex3f(0,0.01,7.5);
    glTexCoord2f(1,0); glVertex3f(0,4,2.5);
    glTexCoord2f(0,1); glVertex3f(-2.5,0.01,2.5);
    glColor3f(1,1,1);
    glNormal3f(5,3.5,2.5);
    glTexCoord2f(0,0); glVertex3f(0,0.01,7.5);
    glTexCoord2f(1,0); glVertex3f(0,4,2.5);
    glTexCoord2f(0,1); glVertex3f(2.5,0.01,2.5);
    glColor3f(1,1,1);
    glNormal3f(-5,3.5,-2.5);
    glTexCoord2f(0,0); glVertex3f(0,0.01,-7.5);
    glTexCoord2f(1,0); glVertex3f(0,4,-2.5);
    glTexCoord2f(0,1); glVertex3f(-2.5,0.01,-2.5);
    glColor3f(1,1,1);
    glNormal3f(5,3.5,-2.5);
    glTexCoord2f(0,0); glVertex3f(0,0.01,-7.5);
    glTexCoord2f(1,0); glVertex3f(0,4,-2.5);
    glTexCoord2f(0,1); glVertex3f(2.5,0.01,-2.5);
    glColor3f(1,1,1);
    glNormal3f(2.5,3.5,-5);
    glTexCoord2f(0,0); glVertex3f(7.5,0.01,0);
    glTexCoord2f(1,0); glVertex3f(2.5,4,0);
    glTexCoord2f(0,1); glVertex3f(2.5,0.01,-2.5);
    glColor3f(1,1,1);
    glNormal3f(2.5,3.5,5);
    glTexCoord2f(0,0); glVertex3f(7.5,0.01,0);
    glTexCoord2f(1,0); glVertex3f(2.5,4,0);
    glTexCoord2f(0,1); glVertex3f(2.5,0.01,2.5);
    glColor3f(1,1,1);
    glNormal3f(-2.5,3.5,-5);
    glTexCoord2f(0,0); glVertex3f(-7.5,0.01,0);
    glTexCoord2f(1,0); glVertex3f(-2.5,4,0);
    glTexCoord2f(0,1); glVertex3f(-2.5,0.01,-2.5);
    glColor3f(1,1,1);
    glNormal3f(-2.5,3.5,5);
    glTexCoord2f(0,0); glVertex3f(-7.5,0.01,0);
    glTexCoord2f(1,0); glVertex3f(-2.5,4,0);
    glTexCoord2f(0,1); glVertex3f(-2.5,0.01,2.5);
    glEnd();
    //Base
    glBegin(GL_QUAD_STRIP);
    glColor3f(1,1,1);
    for (int i=0;i<=360;i+=5)
    {
        float cylTex = i/(2*M_PI);
        glNormal3d((4*cos(i)),0.1,(4*sin(i)));
        glTexCoord2f(cylTex,0);
        glVertex3d((4*cos(i)),-0.01,(4*sin(i)));
        glVertex3d(0,30,0);
    }
    glEnd();
    //Branches
    //Each branch is a long cylinder through the heart of the tree
    double counter = 0;
    for (int i = 4; i<=28; i+=2)
    {
        glBegin(GL_QUAD_STRIP);
        glColor3f(1,1,1);   
        for (int i2=0;i2<=360;i2+=5)
        {
            float cylTex = i2/(2*M_PI);
            glNormal3d(0.1,(0.25*sin(i2)),(0.25*cos(i2)));
            glTexCoord2f(cylTex,0);
            //As the counter increases (every tick of i, not i2), the length of the branches decrease
            //9/12 is proportional to the height of the tree and the number of branches
            glVertex3d(-10+(counter*9/12),(0.25*sin(i2))+i,(0.25*cos(i2)));
            glVertex3d(10-(counter*9/12),(0.25*sin(i2))+i,(0.25*cos(i2)));
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        glColor3f(1,1,1);
        for (int i2=0;i2<=360;i2+=5)
        {
            float cylTex = i2/(2*M_PI);
            glNormal3d(-0.1,0,0);
            glTexCoord2f(cylTex,0);
            glVertex3d(-10+(counter*9/12),(0.25*cos(i2))+i,(0.25*sin(i2)));
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        glColor3f(1,1,1);
        for (int i2=0;i2<=360;i2+=5)
        {
            float cylTex = i2/(2*M_PI);
            glNormal3d(0.1,0,0);
            glTexCoord2f(cylTex,0);
            glVertex3d(10-(counter*9/12),(0.25*cos(i2))+i,(0.25*sin(i2)));
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        glColor3f(1,1,1);   
        for (int i2=0;i2<=360;i2+=5)
        {
            float cylTex = i2/(2*M_PI);
            glNormal3d((0.25*cos(i2)),(0.25*sin(i2)),0.1);
            glTexCoord2f(cylTex,0);
            glVertex3d((0.25*cos(i2)),(0.25*sin(i2))+i,-10+(counter*9/12));
            glVertex3d((0.25*cos(i2)),(0.25*sin(i2))+i,10-(counter*9/12));
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        glColor3f(1,1,1);
        for (int i2=0;i2<=360;i2+=5)
        {
            float cylTex = i2/(2*M_PI);
            glNormal3d(0,0,0.1);
            glTexCoord2f(cylTex,0);
            glVertex3d((0.25*sin(i2)),(0.25*cos(i2))+i,10-(counter*9/12));
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        glColor3f(1,1,1);
        for (int i2=0;i2<=360;i2+=5)
        {
            float cylTex = i2/(2*M_PI);
            glNormal3d(0,0,-0.1);
            glTexCoord2f(cylTex,0);
            glVertex3d((0.25*sin(i2)),(0.25*cos(i2))+i,-10+(counter*9/12));
        }
        glEnd();
        counter++;
    }
    glDisable(GL_TEXTURE_2D);
    //Pines
    //The pines are handled simlarly to the branches (using a counter & 9/10)
    counter = 0;
    glNormal3f(0,1,0);
    for (int i = 4; i<=28; i+=2)
    {
        for (double i2 = 0; i2 <= 9.99-(counter*9/12); i2+=0.5)
        {
            glBegin(GL_LINES);
            //X
            glColor3f(0,1,0);
            glVertex3d(i2,i,0);
            glVertex3d(i2,i+1,0);
            glColor3f(0,1,0);
            glVertex3d(i2,i,0);
            glVertex3d(i2,i+0.25,1);
            glColor3f(0,1,0);
            glVertex3d(i2,i+0.25,-1);
            glVertex3d(i2,i,0);
            glColor3f(0,1,0);
            glVertex3d(-i2,i,0);
            glVertex3d(-i2,i+1,0);
            glColor3f(0,1,0);
            glVertex3d(-i2,i,0);
            glVertex3d(-i2,i+0.25,1);
            glColor3f(0,1,0);
            glVertex3d(-i2,i+0.25,-1);
            glVertex3d(-i2,i,0);
            //Z
            glColor3f(0,1,0);
            glVertex3d(0,i,i2);
            glVertex3d(0,i+1,i2);
            glColor3f(0,1,0);
            glVertex3d(0,i,i2);
            glVertex3d(1,i+0.25,i2);
            glColor3f(0,1,0);
            glVertex3d(-1,i+0.25,i2);
            glVertex3d(0,i,i2);
            glColor3f(0,1,0);
            glVertex3d(0,i,-i2);
            glVertex3d(0,i+1,-i2);
            glColor3f(0,1,0);
            glVertex3d(0,i,-i2);
            glVertex3d(1,i+0.25,-i2);
            glColor3f(0,1,0);
            glVertex3d(-1,i+0.25,-i2);
            glVertex3d(0,i,-i2);
            glEnd();
        }
        counter++;
    }
    glPopMatrix();
}

//This building is based on apartment buildings you'd find in warmer nations which emphasize large patios on each floor
static void highRise(double x, double y, double z, double rot)
{
    float white[] = {1,1,1,1};
    float black[] = {0,0,0,0};
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
    glPushMatrix();
    glTranslated(x,-0.01,z);
    glRotated(rot,0,1,0);
    glScaled(0.1,0.1*y,0.1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,texMode?GL_REPLACE:GL_MODULATE);
    //Base
    glBindTexture(GL_TEXTURE_2D,textures[8]);
    glBegin(GL_QUADS);
    glColor3f(1,1,0);
    glNormal3f(0,0,0.1);
    glTexCoord2f(0,0); glVertex3f(-10,0,10);
    glTexCoord2f(1,0); glVertex3f(10,0,10);
    glTexCoord2f(0,1); glVertex3f(10,10,10);
    glTexCoord2f(1,1); glVertex3f(-10,10,10);
    glColor3f(1,1,0);
    glNormal3f(-0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(-10,0,10);
    glTexCoord2f(1,0); glVertex3f(-10,0,-10);
    glTexCoord2f(0,1); glVertex3f(-10,10,-10);
    glTexCoord2f(1,1); glVertex3f(-10,10,10);
    glColor3f(1,1,0);
    glNormal3f(0,0,-0.1);
    glTexCoord2f(0,0); glVertex3f(-10,0,-10);
    glTexCoord2f(1,0); glVertex3f(10,0,-10);
    glTexCoord2f(0,1); glVertex3f(10,10,-10);
    glTexCoord2f(1,1); glVertex3f(-10,10,-10);
    glColor3f(1,1,0);
    glNormal3f(0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(10,0,10);
    glTexCoord2f(1,0); glVertex3f(10,0,-10);
    glTexCoord2f(0,1); glVertex3f(10,10,-10);
    glTexCoord2f(1,1); glVertex3f(10,10,10);
    glEnd();
    //Doors
    glBindTexture(GL_TEXTURE_2D,textures[7]);
    glBegin(GL_QUADS);
    glColor3f(1,1,1);
    glNormal3f(0,0,0.1);
    glTexCoord2f(1,0); glVertex3f(-4,0,10.01);
    glTexCoord2f(0,0); glVertex3f(4,0,10.01);
    glTexCoord2f(0,1); glVertex3f(4,9,10.01);
    glTexCoord2f(1,1); glVertex3f(-4,9,10.01);
    glEnd();
    //Windows
    glBindTexture(GL_TEXTURE_2D,textures[1]);
    glBegin(GL_QUADS);
    glColor3f(1,1,1);
    glNormal3f(0,0,0.1);
    glTexCoord2f(0,0); glVertex3f(-10,7,10.01);
    glTexCoord2f(1,0); glVertex3f(-5,7,10.01);
    glTexCoord2f(0,1); glVertex3f(-5,3.5,10.01);
    glTexCoord2f(1,1); glVertex3f(-10,3.5,10.01);
    glColor3f(1,1,1);
    glNormal3f(0,0,0.1);
    glTexCoord2f(0,0); glVertex3f(10,7,10.01);
    glTexCoord2f(1,0); glVertex3f(5,7,10.01);
    glTexCoord2f(0,1); glVertex3f(5,3.5,10.01);
    glTexCoord2f(1,1); glVertex3f(10,3.5,10.01);
    glColor3f(1,1,1);
    glNormal3f(0,0,-0.1);
    glTexCoord2f(0,0); glVertex3f(-10,7,-10.01);
    glTexCoord2f(1,0); glVertex3f(10,7,-10.01);
    glTexCoord2f(0,1); glVertex3f(10,3.5,-10.01);
    glTexCoord2f(1,1); glVertex3f(-10,3.5,-10.01);
    glColor3f(1,1,1);
    glNormal3f(0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(10.01,7,-10);
    glTexCoord2f(1,0); glVertex3f(10.01,7,10);
    glTexCoord2f(0,1); glVertex3f(10.01,3.5,10);
    glTexCoord2f(1,1); glVertex3f(10.01,3.5,-10);
    glColor3f(1,1,1);
    glNormal3f(-0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(-10.01,7,-10);
    glTexCoord2f(1,0); glVertex3f(-10.01,7,10);
    glTexCoord2f(0,1); glVertex3f(-10.01,3.5,10);
    glTexCoord2f(1,1); glVertex3f(-10.01,3.5,-10);
    glEnd();
    //Main Tower
    glBindTexture(GL_TEXTURE_2D,textures[8]);
    glBegin(GL_QUADS);
    glColor3f(1,1,1);
    glNormal3f(0,0,0.1);
    glTexCoord2f(0,0); glVertex3f(-6,10,6);
    glTexCoord2f(1,0); glVertex3f(6,10,6);
    glTexCoord2f(0,1); glVertex3f(6,40,6);
    glTexCoord2f(1,1); glVertex3f(-6,40,6);
    glColor3f(1,1,1);
    glNormal3f(-0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(-6,10,6);
    glTexCoord2f(1,0); glVertex3f(-6,10,-6);
    glTexCoord2f(0,1); glVertex3f(-6,40,-6);
    glTexCoord2f(1,1); glVertex3f(-6,40,6);
    glColor3f(1,1,1);
    glNormal3f(0,0,-0.1);
    glTexCoord2f(0,0); glVertex3f(-6,10,-6);
    glTexCoord2f(1,0); glVertex3f(6,10,-6);
    glTexCoord2f(0,1); glVertex3f(6,40,-6);
    glTexCoord2f(1,1); glVertex3f(-6,40,-6);
    glColor3f(1,1,1);
    glNormal3f(0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(6,10,6);
    glTexCoord2f(1,0); glVertex3f(6,10,-6);
    glTexCoord2f(0,1); glVertex3f(6,40,-6);
    glTexCoord2f(1,1); glVertex3f(6,40,6);
    //Base-Tower connection
    glColor3f(1,1,0);
    glNormal3f(0,0.1,0);
    glTexCoord2f(0,0); glVertex3f(-10,10,10);
    glTexCoord2f(1,0); glVertex3f(10,10,10);
    glTexCoord2f(0,1); glVertex3f(10,10,-10);
    glTexCoord2f(1,1); glVertex3f(-10,10,-10);
    //Tower Beams
    //B+Z
    glColor3f(1,1,1);
    glNormal3f(-0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(-1,10,6);
    glTexCoord2f(1,0); glVertex3f(-1,10,10);
    glTexCoord2f(0,1); glVertex3f(-1,40,10);
    glTexCoord2f(1,1); glVertex3f(-1,40,6);
    glColor3f(1,1,1);
    glNormal3f(0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(1,10,6);
    glTexCoord2f(1,0); glVertex3f(1,10,10);
    glTexCoord2f(0,1); glVertex3f(1,40,10);
    glTexCoord2f(1,1); glVertex3f(1,40,6);
    glColor3f(1,1,1);
    glNormal3f(0,0,0.1);
    glTexCoord2f(0,0); glVertex3f(-1,10,10);
    glTexCoord2f(1,0); glVertex3f(1,10,10);
    glTexCoord2f(0,1); glVertex3f(1,40,10);
    glTexCoord2f(1,1); glVertex3f(-1,40,10);
    //B-Z
    glColor3f(1,1,1);
    glNormal3f(-0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(-1,10,-6);
    glTexCoord2f(1,0); glVertex3f(-1,10,-10);
    glTexCoord2f(0,1); glVertex3f(-1,40,-10);
    glTexCoord2f(1,1); glVertex3f(-1,40,-6);
    glColor3f(1,1,1);
    glNormal3f(0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(1,10,-6);
    glTexCoord2f(1,0); glVertex3f(1,10,-10);
    glTexCoord2f(0,1); glVertex3f(1,40,-10);
    glTexCoord2f(1,1); glVertex3f(1,40,-6);
    glColor3f(1,1,1);
    glNormal3f(0,0,-0.1);
    glTexCoord2f(0,0); glVertex3f(-1,10,-10);
    glTexCoord2f(1,0); glVertex3f(1,10,-10);
    glTexCoord2f(0,1); glVertex3f(1,40,-10);
    glTexCoord2f(1,1); glVertex3f(-1,40,-10);
    //B+X
    glColor3f(1,1,1);
    glNormal3f(0,0,-0.1);
    glTexCoord2f(0,0); glVertex3f(6,10,-1);
    glTexCoord2f(1,0); glVertex3f(10,10,-1);
    glTexCoord2f(0,1); glVertex3f(10,40,-1);
    glTexCoord2f(1,1); glVertex3f(6,40,-1);
    glColor3f(1,1,1);
    glNormal3f(0,0,0.1);
    glTexCoord2f(0,0); glVertex3f(6,10,1);
    glTexCoord2f(1,0); glVertex3f(10,10,1);
    glTexCoord2f(0,1); glVertex3f(10,40,1);
    glTexCoord2f(1,1); glVertex3f(6,40,1);
    glColor3f(1,1,1);
    glNormal3f(0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(10,10,-1);
    glTexCoord2f(1,0); glVertex3f(10,10,1);
    glTexCoord2f(0,1); glVertex3f(10,40,1);
    glTexCoord2f(1,1); glVertex3f(10,40,-1);
    //B-X
    glColor3f(1,1,1);
    glNormal3f(0,0,-0.1);
    glTexCoord2f(0,0); glVertex3f(-6,10,-1);
    glTexCoord2f(1,0); glVertex3f(-10,10,-1);
    glTexCoord2f(0,1); glVertex3f(-10,40,-1);
    glTexCoord2f(1,1); glVertex3f(-6,40,-1);
    glColor3f(1,1,1);
    glNormal3f(0,0,0.1);
    glTexCoord2f(0,0); glVertex3f(-6,10,1);
    glTexCoord2f(1,0); glVertex3f(-10,10,1);
    glTexCoord2f(0,1); glVertex3f(-10,40,1);
    glTexCoord2f(1,1); glVertex3f(-6,40,1);
    glColor3f(1,1,1);
    glNormal3f(-0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(-10,10,-1);
    glTexCoord2f(1,0); glVertex3f(-10,10,1);
    glTexCoord2f(0,1); glVertex3f(-10,40,1);
    glTexCoord2f(1,1); glVertex3f(-10,40,-1);
    //Roof
    glColor3f(1,1,1);
    glNormal3f(0,0.1,0);
    glTexCoord2f(0,0); glVertex3f(-10,40.01,-10);
    glTexCoord2f(1,0); glVertex3f(-10,40.01,10);
    glTexCoord2f(0,1); glVertex3f(10,40.01,10);
    glTexCoord2f(1,1); glVertex3f(10,40.01,-10);
    glEnd(); 
    //Patios
    //Each patio has a corner glass door and a saftey railing
    for (int i = 15; i <= 40; i+=5)
    {
        //Floor
        glBindTexture(GL_TEXTURE_2D,textures[8]);
        glBegin(GL_QUADS);
        //+Z+X
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glTexCoord2f(0,0); glVertex3f(1,i,6);
        glTexCoord2f(1,0); glVertex3f(1,i,10);
        glTexCoord2f(0,1); glVertex3f(10,i,10);
        glTexCoord2f(1,1); glVertex3f(10,i,6);
        glColor3f(1,1,1);
        glNormal3f(0,-0.1,0);
        glTexCoord2f(0,0); glVertex3f(1,i-0.5,6);
        glTexCoord2f(1,0); glVertex3f(1,i-0.5,10);
        glTexCoord2f(0,1); glVertex3f(10,i-0.5,10);
        glTexCoord2f(1,1); glVertex3f(10,i-0.5,6);
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glTexCoord2f(0,0); glVertex3f(6,i,1);
        glTexCoord2f(1,0); glVertex3f(6,i,6);
        glTexCoord2f(0,1); glVertex3f(10,i,6);
        glTexCoord2f(1,1); glVertex3f(10,i,1);
        glColor3f(1,1,1);
        glNormal3f(0,-0.1,0);
        glTexCoord2f(0,0); glVertex3f(6,i-0.5,1);
        glTexCoord2f(1,0); glVertex3f(6,i-0.5,6);
        glTexCoord2f(0,1); glVertex3f(10,i-0.5,6);
        glTexCoord2f(1,1); glVertex3f(10,i-0.5,1);
        glColor3f(1,1,1);
        glNormal3f(0,0,0.1);
        glTexCoord2f(0,0); glVertex3f(1,i,10);
        glTexCoord2f(1,0); glVertex3f(1,i-0.5,10);
        glTexCoord2f(0,1); glVertex3f(10,i-0.5,10);
        glTexCoord2f(1,1); glVertex3f(10,i,10);
        glNormal3f(0.1,0,0);
        glTexCoord2f(0,0); glVertex3f(10,i,1);
        glTexCoord2f(1,0); glVertex3f(10,i,10);
        glTexCoord2f(0,1); glVertex3f(10,i-0.5,10);
        glTexCoord2f(1,1); glVertex3f(10,i-0.5,1);
        //+Z-X
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glTexCoord2f(0,0); glVertex3f(-1,i,6);
        glTexCoord2f(1,0); glVertex3f(-1,i,10);
        glTexCoord2f(0,1); glVertex3f(-10,i,10);
        glTexCoord2f(1,1); glVertex3f(-10,i,6);
        glColor3f(1,1,1);
        glNormal3f(0,-0.1,0);
        glTexCoord2f(0,0); glVertex3f(-1,i-0.5,6);
        glTexCoord2f(1,0); glVertex3f(-1,i-0.5,10);
        glTexCoord2f(0,1); glVertex3f(-10,i-0.5,10);
        glTexCoord2f(1,1); glVertex3f(-10,i-0.5,6);
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glTexCoord2f(0,0); glVertex3f(-6,i,1);
        glTexCoord2f(1,0); glVertex3f(-6,i,6);
        glTexCoord2f(0,1); glVertex3f(-10,i,6);
        glTexCoord2f(1,1); glVertex3f(-10,i,1);
        glColor3f(1,1,1);
        glNormal3f(0,-0.1,0);
        glTexCoord2f(0,0); glVertex3f(-6,i-0.5,1);
        glTexCoord2f(1,0); glVertex3f(-6,i-0.5,6);
        glTexCoord2f(0,1); glVertex3f(-10,i-0.5,6);
        glTexCoord2f(1,1); glVertex3f(-10,i-0.5,1);
        glColor3f(1,1,1);
        glNormal3f(0,0,0.1);
        glTexCoord2f(0,0); glVertex3f(-1,i,10);
        glTexCoord2f(1,0); glVertex3f(-1,i-0.5,10);
        glTexCoord2f(0,1); glVertex3f(-10,i-0.5,10);
        glTexCoord2f(1,1); glVertex3f(-10,i,10);
        glColor3f(1,1,1);
        glNormal3f(-0.1,0,0);
        glTexCoord2f(0,0); glVertex3f(-10,i,1);
        glTexCoord2f(1,0); glVertex3f(-10,i,10);
        glTexCoord2f(0,1); glVertex3f(-10,i-0.5,10);
        glTexCoord2f(1,1); glVertex3f(-10,i-0.5,1);
        //-Z+X
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glTexCoord2f(0,0); glVertex3f(1,i,-6);
        glTexCoord2f(1,0); glVertex3f(1,i,-10);
        glTexCoord2f(0,1); glVertex3f(10,i,-10);
        glTexCoord2f(1,1); glVertex3f(10,i,-6);
        glColor3f(1,1,1);
        glNormal3f(0,-0.1,0);
        glTexCoord2f(0,0); glVertex3f(1,i-0.5,-6);
        glTexCoord2f(1,0); glVertex3f(1,i-0.5,-10);
        glTexCoord2f(0,1); glVertex3f(10,i-0.5,-10);
        glTexCoord2f(1,1); glVertex3f(10,i-0.5,-6);
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glTexCoord2f(0,0); glVertex3f(6,i,-1);
        glTexCoord2f(1,0); glVertex3f(6,i,-6);
        glTexCoord2f(0,1); glVertex3f(10,i,-6);
        glTexCoord2f(1,1); glVertex3f(10,i,-1);
        glColor3f(1,1,1);
        glNormal3f(0,-0.1,0);
        glTexCoord2f(0,0); glVertex3f(6,i-0.5,-1);
        glTexCoord2f(1,0); glVertex3f(6,i-0.5,-6);
        glTexCoord2f(0,1); glVertex3f(10,i-0.5,-6);
        glTexCoord2f(1,1); glVertex3f(10,i-0.5,-1);
        glColor3f(1,1,1);
        glNormal3f(0,0,-0.1);
        glTexCoord2f(0,0); glVertex3f(1,i,-10);
        glTexCoord2f(1,0); glVertex3f(1,i-0.5,-10);
        glTexCoord2f(0,1); glVertex3f(10,i-0.5,-10);
        glTexCoord2f(1,1); glVertex3f(10,i,-10);
        glColor3f(1,1,1);
        glNormal3f(0.1,0,0);
        glTexCoord2f(0,0); glVertex3f(10,i,-1);
        glTexCoord2f(1,0); glVertex3f(10,i,-10);
        glTexCoord2f(0,1); glVertex3f(10,i-0.5,-10);
        glTexCoord2f(1,1); glVertex3f(10,i-0.5,-1);
        //-Z-X
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glTexCoord2f(0,0); glVertex3f(-1,i,-6);
        glTexCoord2f(1,0); glVertex3f(-1,i,-10);
        glTexCoord2f(0,1); glVertex3f(-10,i,-10);
        glTexCoord2f(1,1); glVertex3f(-10,i,-6);
        glColor3f(1,1,1);
        glNormal3f(0,-0.1,0);
        glTexCoord2f(0,0); glVertex3f(-1,i-0.5,-6);
        glTexCoord2f(1,0); glVertex3f(-1,i-0.5,-10);
        glTexCoord2f(0,1); glVertex3f(-10,i-0.5,-10);
        glTexCoord2f(1,1); glVertex3f(-10,i-0.5,-6);
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glTexCoord2f(0,0); glVertex3f(-6,i,-1);
        glTexCoord2f(1,0); glVertex3f(-6,i,-6);
        glTexCoord2f(0,1); glVertex3f(-10,i,-6);
        glTexCoord2f(1,1); glVertex3f(-10,i,-1);
        glColor3f(1,1,1);
        glNormal3f(0,-0.1,0);
        glTexCoord2f(0,0); glVertex3f(-6,i-0.5,-1);
        glTexCoord2f(1,0); glVertex3f(-6,i-0.5,-6);
        glTexCoord2f(0,1); glVertex3f(-10,i-0.5,-6);
        glTexCoord2f(1,1); glVertex3f(-10,i-0.5,-1);
        glColor3f(1,1,1);
        glNormal3f(0,0,-0.1);
        glTexCoord2f(0,0); glVertex3f(-1,i,-10);
        glTexCoord2f(1,0); glVertex3f(-1,i-0.5,-10);
        glTexCoord2f(0,1); glVertex3f(-10,i-0.5,-10);
        glTexCoord2f(1,1); glVertex3f(-10,i,-10);
        glColor3f(1,1,1);
        glNormal3f(-0.1,0,0);
        glTexCoord2f(0,0); glVertex3f(-10,i,-1);
        glTexCoord2f(1,0); glVertex3f(-10,i,-10);
        glTexCoord2f(0,1); glVertex3f(-10,i-0.5,-10);
        glTexCoord2f(1,1); glVertex3f(-10,i-0.5,-1);
        glEnd();
        if (i != 40)
        {
            //Doors
            glBindTexture(GL_TEXTURE_2D,textures[1]);
            glBegin(GL_QUADS);
            //+Z+X
            glColor3f(0,1,1);
            glNormal3f(0,0,0.1);
            glTexCoord2f(0,0); glVertex3f(4,i,6.01);
            glTexCoord2f(1,0); glVertex3f(6,i,6.01);
            glTexCoord2f(0,1); glVertex3f(6,i+3.5,6.01);
            glTexCoord2f(1,1); glVertex3f(4,i+3.5,6.01);
            glColor3f(0,1,1);
            glNormal3f(0.1,0,0);
            glTexCoord2f(0,0); glVertex3f(6.01,i,4);
            glTexCoord2f(1,0); glVertex3f(6.01,i,6);
            glTexCoord2f(0,1); glVertex3f(6.01,i+3.5,6);
            glTexCoord2f(1,1); glVertex3f(6.01,i+3.5,4);
            //-Z+X
            glColor3f(0,1,1);
            glNormal3f(0,0,-0.1);
            glTexCoord2f(0,0); glVertex3f(4,i,-6.01);
            glTexCoord2f(1,0); glVertex3f(6,i,-6.01);
            glTexCoord2f(0,1); glVertex3f(6,i+3.5,-6.01);
            glTexCoord2f(1,1); glVertex3f(4,i+3.5,-6.01);
            glColor3f(0,1,1);
            glNormal3f(0.1,0,0);
            glTexCoord2f(0,0); glVertex3f(6.01,i,-4);
            glTexCoord2f(1,0); glVertex3f(6.01,i,-6);
            glTexCoord2f(0,1); glVertex3f(6.01,i+3.5,-6);
            glTexCoord2f(1,1); glVertex3f(6.01,i+3.5,-4);
            //+Z-X
            glColor3f(0,1,1);
            glNormal3f(0,0,0.1);
            glTexCoord2f(0,0); glVertex3f(-4,i,6.01);
            glTexCoord2f(1,0); glVertex3f(-6,i,6.01);
            glTexCoord2f(0,1); glVertex3f(-6,i+3.5,6.01);
            glTexCoord2f(1,1); glVertex3f(-4,i+3.5,6.01);
            glColor3f(0,1,1);
            glNormal3f(-0.1,0,0);
            glTexCoord2f(0,0); glVertex3f(-6.01,i,4);
            glTexCoord2f(1,0); glVertex3f(-6.01,i,6);
            glTexCoord2f(0,1); glVertex3f(-6.01,i+3.5,6);
            glTexCoord2f(1,1); glVertex3f(-6.01,i+3.5,4);
            //-Z-X
            glColor3f(0,1,1);
            glNormal3f(0,0,-0.1);
            glTexCoord2f(0,0); glVertex3f(-4,i,-6.01);
            glTexCoord2f(1,0); glVertex3f(-6,i,-6.01);
            glTexCoord2f(0,1); glVertex3f(-6,i+3.5,-6.01);
            glTexCoord2f(1,1); glVertex3f(-4,i+3.5,-6.01);
            glColor3f(0,1,1);
            glNormal3f(-0.1,0,0);
            glTexCoord2f(0,0); glVertex3f(-6.01,i,-4);
            glTexCoord2f(1,0); glVertex3f(-6.01,i,-6);
            glTexCoord2f(0,1); glVertex3f(-6.01,i+3.5,-6);
            glTexCoord2f(1,1); glVertex3f(-6.01,i+3.5,-4);
            glEnd();
            //Saftey Rail
            //+Z+X
            glBindTexture(GL_TEXTURE_2D,textures[0]);
            glBegin(GL_QUAD_STRIP);
            glColor3f(0.5,0.5,0.5);   
            for (int i2=0;i2<=360;i2+=5)
            {
                float cylTex = i2/(2*M_PI);
                glNormal3d(0.1,(0.25*sin(i2)),(0.25*cos(i2)));
                glTexCoord2f(cylTex,0);
                glVertex3d(1,(0.25*sin(i2))+i+1.75,(0.25*cos(i2))+9.5);
                glVertex3d(9.5,(0.25*sin(i2))+i+1.75,(0.25*cos(i2))+9.5);
            }
            glEnd();
            glBegin(GL_QUAD_STRIP);
            glColor3f(0.5,0.5,0.5);   
            for (int i2=0;i2<=360;i2+=5)
            {
                float cylTex = i2/(2*M_PI);
                glNormal3d((0.25*cos(i2)),(0.25*sin(i2)),0.1);
                glTexCoord2f(cylTex,0);
                glVertex3d((0.25*cos(i2))+9.5,(0.25*sin(i2))+i+1.75,1);
                glVertex3d((0.25*cos(i2))+9.5,(0.25*sin(i2))+i+1.75,9.5);
            }
            glEnd();
            //+Z-X
            glBegin(GL_QUAD_STRIP);
            glColor3f(0.5,0.5,0.5);   
            for (int i2=0;i2<=360;i2+=5)
            {
                float cylTex = i2/(2*M_PI);
                glNormal3d(-0.1,(0.25*sin(i2)),(0.25*cos(i2)));
                glTexCoord2f(cylTex,0);
                glVertex3d(-1,(0.25*sin(i2))+i+1.75,(0.25*cos(i2))+9.5);
                glVertex3d(-9.5,(0.25*sin(i2))+i+1.75,(0.25*cos(i2))+9.5);
            }
            glEnd();
            glBegin(GL_QUAD_STRIP);
            glColor3f(0.5,0.5,0.5);   
            for (int i2=0;i2<=360;i2+=5)
            {
                float cylTex = i2/(2*M_PI);
                glNormal3d((0.25*cos(i2)),(0.25*sin(i2)),0.1);
                glTexCoord2f(cylTex,0);
                glVertex3d((0.25*cos(i2))-9.5,(0.25*sin(i2))+i+1.75,1);
                glVertex3d((0.25*cos(i2))-9.5,(0.25*sin(i2))+i+1.75,9.5);
            }
            glEnd();
            //-Z+X
            glBegin(GL_QUAD_STRIP);
            glColor3f(0.5,0.5,0.5);   
            for (int i2=0;i2<=360;i2+=5)
            {
                float cylTex = i2/(2*M_PI);
                glNormal3d(0.1,(0.25*sin(i2)),(0.25*cos(i2)));
                glTexCoord2f(cylTex,0);
                glVertex3d(1,(0.25*sin(i2))+i+1.75,(0.25*cos(i2))-9.5);
                glVertex3d(9.5,(0.25*sin(i2))+i+1.75,(0.25*cos(i2))-9.5);
            }
            glEnd();
            glBegin(GL_QUAD_STRIP);
            glColor3f(0.5,0.5,0.5);   
            for (int i2=0;i2<=360;i2+=5)
            {
                float cylTex = i2/(2*M_PI);
                glNormal3d((0.25*cos(i2)),(0.25*sin(i2)),-0.1);
                glTexCoord2f(cylTex,0);
                glVertex3d((0.25*cos(i2))+9.5,(0.25*sin(i2))+i+1.75,-1);
                glVertex3d((0.25*cos(i2))+9.5,(0.25*sin(i2))+i+1.75,-9.5);
            }
            glEnd();
            //-Z-X
            glBegin(GL_QUAD_STRIP);
            glColor3f(0.5,0.5,0.5);   
            for (int i2=0;i2<=360;i2+=5)
            {
                float cylTex = i2/(2*M_PI);
                glNormal3d(-0.1,(0.25*sin(i2)),(0.25*cos(i2)));
                glTexCoord2f(cylTex,0);
                glVertex3d(-1,(0.25*sin(i2))+i+1.75,(0.25*cos(i2))-9.5);
                glVertex3d(-9.5,(0.25*sin(i2))+i+1.75,(0.25*cos(i2))-9.5);
            }
            glEnd();
            glBegin(GL_QUAD_STRIP);
            glColor3f(0.5,0.5,0.5);   
            for (int i2=0;i2<=360;i2+=5)
            {
                float cylTex = i2/(2*M_PI);
                glNormal3d((0.25*cos(i2)),(0.25*sin(i2)),-0.1);
                glTexCoord2f(cylTex,0);
                glVertex3d((0.25*cos(i2))-9.5,(0.25*sin(i2))+i+1.75,-1);
                glVertex3d((0.25*cos(i2))-9.5,(0.25*sin(i2))+i+1.75,-9.5);
            }
            glEnd();
            for (int i3 = 2; i3 <= 8; i3+=2)
            {
                //+Z+X
                glBegin(GL_QUAD_STRIP);
                glColor3f(0.5,0.5,0.5); 
                for (int i2=0;i2<=360;i2+=5)
                {
                    float cylTex = i2/(2*M_PI);
                    glNormal3d((0.125*cos(i2)),0.1,(0.125*sin(i2)));
                    glTexCoord2f(cylTex,0);
                    glVertex3d((0.125*cos(i2))+i3,i,(0.125*sin(i2))+9.5);
                    glVertex3d((0.125*cos(i2))+i3,i+1.75,(0.125*sin(i2))+9.5);
                }
                glEnd();
                glBegin(GL_QUAD_STRIP);
                glColor3f(0.5,0.5,0.5); 
                for (int i2=0;i2<=360;i2+=5)
                {
                    float cylTex = i2/(2*M_PI);
                    glNormal3d((0.125*cos(i2)),0.1,(0.125*sin(i2)));
                    glTexCoord2f(cylTex,0);
                    glVertex3d((0.125*cos(i2))+9.5,i,(0.125*sin(i2))+i3);
                    glVertex3d((0.125*cos(i2))+9.5,i+1.75,(0.125*sin(i2))+i3);
                }
                glEnd();
                //+Z-X
                glBegin(GL_QUAD_STRIP);
                glColor3f(0.5,0.5,0.5); 
                for (int i2=0;i2<=360;i2+=5)
                {
                    float cylTex = i2/(2*M_PI);
                    glNormal3d((0.125*cos(i2)),0.1,(0.125*sin(i2)));
                    glTexCoord2f(cylTex,0);
                    glVertex3d((0.125*cos(i2))-i3,i,(0.125*sin(i2))+9.5);
                    glVertex3d((0.125*cos(i2))-i3,i+1.75,(0.125*sin(i2))+9.5);
                }
                glEnd();
                glBegin(GL_QUAD_STRIP);
                glColor3f(0.5,0.5,0.5); 
                for (int i2=0;i2<=360;i2+=5)
                {
                    float cylTex = i2/(2*M_PI);
                    glNormal3d((0.125*cos(i2)),0.1,(0.125*sin(i2)));
                    glTexCoord2f(cylTex,0);
                    glVertex3d((0.125*cos(i2))-9.5,i,(0.125*sin(i2))+i3);
                    glVertex3d((0.125*cos(i2))-9.5,i+1.75,(0.125*sin(i2))+i3);
                }
                glEnd();
                //-Z+X
                glBegin(GL_QUAD_STRIP);
                glColor3f(0.5,0.5,0.5); 
                for (int i2=0;i2<=360;i2+=5)
                {
                    float cylTex = i2/(2*M_PI);
                    glNormal3d((0.125*cos(i2)),0.1,(0.125*sin(i2)));
                    glTexCoord2f(cylTex,0);
                    glVertex3d((0.125*cos(i2))+i3,i,(0.125*sin(i2))-9.5);
                    glVertex3d((0.125*cos(i2))+i3,i+1.75,(0.125*sin(i2))-9.5);
                }
                glEnd();
                glBegin(GL_QUAD_STRIP);
                glColor3f(0.5,0.5,0.5); 
                for (int i2=0;i2<=360;i2+=5)
                {
                    float cylTex = i2/(2*M_PI);
                    glNormal3d((0.125*cos(i2)),0.1,(0.125*sin(i2)));
                    glTexCoord2f(cylTex,0);
                    glVertex3d((0.125*cos(i2))+9.5,i,(0.125*sin(i2))-i3);
                    glVertex3d((0.125*cos(i2))+9.5,i+1.75,(0.125*sin(i2))-i3);
                }
                glEnd();
                //-Z-X
                glBegin(GL_QUAD_STRIP);
                glColor3f(0.5,0.5,0.5); 
                for (int i2=0;i2<=360;i2+=5)
                {
                    float cylTex = i2/(2*M_PI);
                    glNormal3d((0.125*cos(i2)),0.1,(0.125*sin(i2)));
                    glTexCoord2f(cylTex,0);
                    glVertex3d((0.125*cos(i2))-i3,i,(0.125*sin(i2))-9.5);
                    glVertex3d((0.125*cos(i2))-i3,i+1.75,(0.125*sin(i2))-9.5);
                }
                glEnd();
                glBegin(GL_QUAD_STRIP);
                glColor3f(0.5,0.5,0.5); 
                for (int i2=0;i2<=360;i2+=5)
                {
                    float cylTex = i2/(2*M_PI);
                    glNormal3d((0.125*cos(i2)),0.1,(0.125*sin(i2)));
                    glTexCoord2f(cylTex,0);
                    glVertex3d((0.125*cos(i2))-9.5,i,(0.125*sin(i2))-i3);
                    glVertex3d((0.125*cos(i2))-9.5,i+1.75,(0.125*sin(i2))-i3);
                }
                glEnd();
            }
        }
    }
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

//A suburban style house
static void subHouse(double x, double y, double z, double rot)
{
    float white[] = {1,1,1,1};
    float black[] = {0,0,0,0};
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
    glPushMatrix();
    //y is used in the coordinates rather than in translated so that nothing is underground
    glTranslated(x,-0.01,z);
    glRotated(rot,0,1,0);
    glScaled(0.1,0.1,0.1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,texMode?GL_REPLACE:GL_MODULATE);
    //4 walls
    glBindTexture(GL_TEXTURE_2D,textures[5]);
    glBegin(GL_QUADS);
    glColor3f(1,0,1);
    glNormal3f(0,0,0.1);
    glTexCoord2f(0,0); glVertex3f(-10,0,10);
    glTexCoord2f(1,0); glVertex3f(10,0,10);
    glTexCoord2f(0,1); glVertex3f(10,y,10);
    glTexCoord2f(1,1); glVertex3f(-10,y,10);
    glColor3f(1,0,1);
    glNormal3f(-0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(-10,0,10);
    glTexCoord2f(1,0); glVertex3f(-10,0,-10);
    glTexCoord2f(0,1); glVertex3f(-10,y,-10);
    glTexCoord2f(1,1); glVertex3f(-10,y,10);
    glColor3f(1,0,1);
    glNormal3f(0,0,-0.1);
    glTexCoord2f(0,0); glVertex3f(-10,0,-10);
    glTexCoord2f(1,0); glVertex3f(10,0,-10);
    glTexCoord2f(0,1); glVertex3f(10,y,-10);
    glTexCoord2f(1,1); glVertex3f(-10,y,-10);
    glColor3f(1,0,1);
    glNormal3f(0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(10,0,10);
    glTexCoord2f(1,0); glVertex3f(10,0,-10);
    glTexCoord2f(0,1); glVertex3f(10,y,-10);
    glTexCoord2f(1,1); glVertex3f(10,y,10);
    glEnd();
    //Roof
    glBindTexture(GL_TEXTURE_2D,textures[3]);
    glBegin(GL_TRIANGLES);
    glColor3f(1,0.5,0);
    glNormal3f(0,((y-(y/2))/2)+0.1,5+0.1);
    glTexCoord2f(0,0); glVertex3f(10,y,10);
    glTexCoord2f(1,0); glVertex3f(-10,y,10);
    glTexCoord2f(0,1); glVertex3f(0,y+(y/2),0);
    glColor3f(1,0.5,0);
    glNormal3f(0,((y-(y/2))/2)+0.1,-5-0.1);
    glTexCoord2f(0,0); glVertex3f(10,y,-10);
    glTexCoord2f(1,0); glVertex3f(-10,y,-10);
    glTexCoord2f(0,1); glVertex3f(0,y+(y/2),0);
    glColor3f(1,0.5,0);
    glNormal3f(-5-0.1,((y-(y/2))/2)+0.1,0);
    glTexCoord2f(0,0); glVertex3f(-10,y,-10);
    glTexCoord2f(1,0); glVertex3f(-10,y,10);
    glTexCoord2f(0,1); glVertex3f(0,y+(y/2),0);
    glColor3f(1,0.5,0);
    glNormal3f(5+0.1,((y-(y/2))/2)+0.1,0);
    glTexCoord2f(0,0); glVertex3f(10,y,-10);
    glTexCoord2f(1,0); glVertex3f(10,y,10);
    glTexCoord2f(0,1); glVertex3f(0,y+(y/2),0);
    glEnd();
    //Round Windows
    glBindTexture(GL_TEXTURE_2D,textures[5]);
    glBegin(GL_QUAD_STRIP);
    glColor3f(1,0,1);
    for (int i=0;i<=360;i+=5)
    {
        float cylTex = i/(2*M_PI);
        glNormal3d((2*cos(i))+5.01,(2*sin(i))+(y/2),11.01);
        glTexCoord2f(cylTex,0);
        glVertex3d((2*cos(i))+5,(2*sin(i))+(y/2),10);
        glVertex3d((2*cos(i))+5,(2*sin(i))+(y/2),10.5);
    }
    glEnd();
    glBindTexture(GL_TEXTURE_2D,textures[1]);
    glBegin(GL_QUAD_STRIP);
    glColor3f(1,1,1);
    for (int i=0;i<=360;i+=5)
    {
        float cylTex = i/(2*M_PI);
        glNormal3d(0,0,0.1);
        glTexCoord2f(cylTex,0);
        glVertex3d((1.8*sin(i))+5,(1.8*cos(i))+(y/2),10.2);
    }
    glEnd();
    glBindTexture(GL_TEXTURE_2D,textures[5]);
    glBegin(GL_QUAD_STRIP);
    glColor3f(1,0,1);
    for (int i=0;i<=360;i+=5)
    {
        float cylTex = i/(2*M_PI);
        glNormal3d((2*cos(i))-5.01,(2*sin(i))+(y/2),11.01);
        glTexCoord2f(cylTex,0);
        glVertex3d((2*cos(i))-5,(2*sin(i))+(y/2),10);
        glVertex3d((2*cos(i))-5,(2*sin(i))+(y/2),10.5);
    }
    glEnd();
    glBindTexture(GL_TEXTURE_2D,textures[1]);
    glBegin(GL_QUAD_STRIP);
    glColor3f(1,1,1);
    for (int i=0;i<=360;i+=5)
    {
        float cylTex = i/(2*M_PI);
        glNormal3d(0,0,0.1);
        glTexCoord2f(cylTex,0);
        glVertex3d((1.8*sin(i))-5,(1.8*cos(i))+(y/2),10.2);
    }
    glEnd();
    //Rectangular Windows
    glBindTexture(GL_TEXTURE_2D,textures[1]);
    glBegin(GL_QUADS);
    glColor3f(1,1,1);
    glNormal3f(0,0,-0.1);
    glTexCoord2f(0,0); glVertex3f(5,(y/4),-10.01);
    glTexCoord2f(1,0); glVertex3f(-5,(y/4),-10.01);
    glTexCoord2f(0,1); glVertex3f(-5,((3*y)/4),-10.01);
    glTexCoord2f(1,1); glVertex3f(5,((3*y)/4),-10.01);
    glColor3f(1,1,1);
    glNormal3f(-0.1,0,0);
    glTexCoord2f(0,0); glVertex3f(-10.01,(y/4),2.5);
    glTexCoord2f(1,0); glVertex3f(-10.01,(y/4),-2.5);
    glTexCoord2f(0,1); glVertex3f(-10.01,((3*y)/4),-2.5);
    glTexCoord2f(1,1); glVertex3f(-10.01,((3*y)/4),2.5);
    glEnd();
    //Front Door
    glBindTexture(GL_TEXTURE_2D,textures[4]);
    glBegin(GL_QUADS);
    glColor3f(1,1,1);
    glNormal3f(0,0,0.1);
    glTexCoord2f(0,0); glVertex3f(2.5,0,10.01);
    glTexCoord2f(1,0); glVertex3f(-2.5,0,10.01);
    glTexCoord2f(1,1); glVertex3f(-2.5,((3*y)/4),10.01);
    glTexCoord2f(0,1); glVertex3f(2.5,((3*y)/4),10.01);
    glEnd(); 
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

//A road with curbs, white lines, and stop signs
//Intersections can be set to 0 or 1 to determine if it's a 4 way stop or a straightaway
static void road(double x, double z, double rot, int intersection)
{
    float white[] = {1,1,1,1};
    float black[] = {0,0,0,0};
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);
    glPushMatrix();
    glTranslated(x,-0.01,z);
    glRotated(rot,0,1,0);
    glScaled(0.1,0.1,0.1);
    if (intersection == 0)
    {
        glBegin(GL_QUADS);
        //Main street
        glColor3f(0,0,0);
        glNormal3f(0,0.1,0);
        glVertex3f(7,-0.02,10);
        glVertex3f(-7,-0.02,10);
        glVertex3f(-7,-0.02,-10);
        glVertex3f(7,-0.02,-10);
        //Sidewalks
        glColor3f(0.8,0.8,0.8);
        glNormal3f(0,0.1,0);
        glVertex3f(-7,-0.02,10);
        glVertex3f(-10,-0.02,10);
        glVertex3f(-10,-0.02,-10);
        glVertex3f(-7,-0.02,-10);
        glColor3f(0.8,0.8,0.8);
        glNormal3f(0,0.1,0);
        glVertex3f(10,-0.02,10);
        glVertex3f(7,-0.02,10);
        glVertex3f(7,-0.02,-10);
        glVertex3f(10,-0.02,-10);
        glEnd();
        //White Lines
        for (int i = -10; i<10; i+=2)
        {
            glBegin(GL_QUADS);
            glColor3f(1,1,1);
            glNormal3f(0,0.1,0);
            glVertex3f(0.8,-0.01,i+1);
            glVertex3f(-0.8,-0.01,i+1);
            glVertex3f(-0.8,-0.01,i);
            glVertex3f(0.8,-0.01,i);
            glEnd();
        }
        glPopMatrix();
    }
    else
    {
        glBegin(GL_QUADS);
        //Main street
        glColor3f(0,0,0);
        glNormal3f(0,0.1,0);
        glVertex3f(7,-0.02,10);
        glVertex3f(-7,-0.02,10);
        glVertex3f(-7,-0.02,-10);
        glVertex3f(7,-0.02,-10);
        glColor3f(0,0,0);
        glNormal3f(0,0.1,0);
        glVertex3f(-7,-0.02,7);
        glVertex3f(-10,-0.02,7);
        glVertex3f(-10,-0.02,-7);
        glVertex3f(-7,-0.02,-7);
        glColor3f(0,0,0);
        glNormal3f(0,0.1,0);
        glVertex3f(10,-0.02,7);
        glVertex3f(7,-0.02,7);
        glVertex3f(7,-0.02,-7);
        glVertex3f(10,-0.02,-7);
        //Curbs
        glColor3f(0.8,0.8,0.8);
        glNormal3f(0,0.1,0);
        glVertex3f(10,-0.02,10);
        glVertex3f(7,-0.02,10);
        glVertex3f(7,-0.02,7);
        glVertex3f(10,-0.02,7);
        glColor3f(0.8,0.8,0.8);
        glNormal3f(0,0.1,0);
        glVertex3f(10,-0.02,-10);
        glVertex3f(7,-0.02,-10);
        glVertex3f(7,-0.02,-7);
        glVertex3f(10,-0.02,-7);
        glColor3f(0.8,0.8,0.8);
        glNormal3f(0,0.1,0);
        glVertex3f(-10,-0.02,10);
        glVertex3f(-7,-0.02,10);
        glVertex3f(-7,-0.02,7);
        glVertex3f(-10,-0.02,7);
        glColor3f(0.8,0.8,0.8);
        glNormal3f(0,0.1,0);
        glVertex3f(-10,-0.02,-10);
        glVertex3f(-7,-0.02,-10);
        glVertex3f(-7,-0.02,-7);
        glVertex3f(-10,-0.02,-7);
        //Stop lines
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glVertex3f(7,-0.01,7);
        glVertex3f(0,-0.01,7);
        glVertex3f(0,-0.01,8);
        glVertex3f(7,-0.01,8);     
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glVertex3f(7,-0.01,-7);
        glVertex3f(7,-0.01,0);
        glVertex3f(8,-0.01,0);
        glVertex3f(8,-0.01,-7);
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glVertex3f(-7,-0.01,7);
        glVertex3f(-7,-0.01,0);
        glVertex3f(-8,-0.01,0);
        glVertex3f(-8,-0.01,7);
        glColor3f(1,1,1);
        glNormal3f(0,0.1,0);
        glVertex3f(-7,-0.01,-7);
        glVertex3f(-0,-0.01,-7);
        glVertex3f(-0,-0.01,-8);
        glVertex3f(-7,-0.01,-8);
        glEnd();
        //Stop Signs
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,texMode?GL_REPLACE:GL_MODULATE);
        //The signs are 3d tubes rather than just lines to make the signs more impactful
        glBindTexture(GL_TEXTURE_2D,textures[0]);
        glBegin(GL_QUAD_STRIP);
        glColor3f(0.5,0.5,0.5);
        for (int i=0;i<=360;i+=5)
        {
            float cylTex = i/(2*M_PI);
            glNormal3d((0.25*cos(i)),0.1,(0.25*sin(i)));
            glTexCoord2f(cylTex,0);
            glVertex3d((0.25*cos(i))+8.5,-0.01,(0.25*sin(i))+8.5);
            glVertex3d((0.25*cos(i))+8.5,8,(0.25*sin(i))+8.5);
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        glColor3f(0.5,0.5,0.5);
        for (int i=0;i<=360;i+=5)
        {
            float cylTex = i/(2*M_PI);
            glNormal3d((0.25*cos(i)),0.1,(0.25*sin(i)));
            glTexCoord2f(cylTex,0);
            glVertex3d((0.25*cos(i))-8.5,-0.01,(0.25*sin(i))+8.5);
            glVertex3d((0.25*cos(i))-8.5,8,(0.25*sin(i))+8.5);
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        glColor3f(0.5,0.5,0.5);
        for (int i=0;i<=360;i+=5)
        {
            float cylTex = i/(2*M_PI);
            glNormal3d((0.25*cos(i)),0.1,(0.25*sin(i)));
            glTexCoord2f(cylTex,0);
            glVertex3d((0.25*cos(i))+8.5,-0.01,(0.25*sin(i))-8.5);
            glVertex3d((0.25*cos(i))+8.5,8,(0.25*sin(i))-8.5);
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        glColor3f(0.5,0.5,0.5);
        for (int i=0;i<=360;i+=5)
        {
            float cylTex = i/(2*M_PI);
            glNormal3d((0.25*cos(i)),0.1,(0.25*sin(i)));
            glTexCoord2f(cylTex,0);
            glVertex3d((0.25*cos(i))-8.5,-0.01,(0.25*sin(i))-8.5);
            glVertex3d((0.25*cos(i))-8.5,8,(0.25*sin(i))-8.5);
        }
        glEnd();
        glBegin(GL_POLYGON);
        glColor3f(1,0,0);
        glNormal3f(0,0,0.1);
        glTexCoord3f(0,0,0); glVertex3f(9.25,8,8.77);
        glTexCoord3f(1,0,0); glVertex3f(10,7,8.77);
        glTexCoord3f(0,1,0); glVertex3f(10,6,8.77);
        glTexCoord3f(1,1,0); glVertex3f(9.25,5,8.77);
        glTexCoord3f(0,0,0); glVertex3f(7.75,5,8.77);
        glTexCoord3f(1,0,0); glVertex3f(7,6,8.77);
        glTexCoord3f(0,1,0); glVertex3f(7,7,8.77);
        glTexCoord3f(1,1,0); glVertex3f(7.75,8,8.77);
        glEnd();
        glBegin(GL_POLYGON);
        glColor3f(1,0,0);
        glNormal3f(0,0,-0.1);
        glTexCoord3f(0,0,0); glVertex3f(-9.25,8,-8.77);
        glTexCoord3f(1,0,0); glVertex3f(-10,7,-8.77);
        glTexCoord3f(0,1,0); glVertex3f(-10,6,-8.77);
        glTexCoord3f(1,1,0); glVertex3f(-9.25,5,-8.77);
        glTexCoord3f(0,0,0); glVertex3f(-7.75,5,-8.77);
        glTexCoord3f(1,0,0); glVertex3f(-7,6,-8.77);
        glTexCoord3f(0,1,0); glVertex3f(-7,7,-8.77);
        glTexCoord3f(1,1,0); glVertex3f(-7.75,8,-8.77);
        glEnd();
        glBegin(GL_POLYGON);
        glColor3f(1,0,0);
        glNormal3f(-0.1,0,0);
        glTexCoord3f(0,0,0); glVertex3f(-8.77,8,9.25);
        glTexCoord3f(1,0,0); glVertex3f(-8.77,7,10);
        glTexCoord3f(0,1,0); glVertex3f(-8.77,6,10);
        glTexCoord3f(1,1,0); glVertex3f(-8.77,5,9.25);
        glTexCoord3f(0,0,0); glVertex3f(-8.77,5,7.75);
        glTexCoord3f(1,0,0); glVertex3f(-8.77,6,7);
        glTexCoord3f(0,1,0); glVertex3f(-8.77,7,7);
        glTexCoord3f(1,1,0); glVertex3f(-8.77,8,7.75);
        glEnd();
        glBegin(GL_POLYGON);
        glColor3f(1,0,0);
        glNormal3f(0.1,0,0);
        glTexCoord3f(0,0,0); glVertex3f(8.77,8,-9.25);
        glTexCoord3f(1,0,0); glVertex3f(8.77,7,-10);
        glTexCoord3f(0,1,0); glVertex3f(8.77,6,-10);
        glTexCoord3f(1,1,0); glVertex3f(8.77,5,-9.25);
        glTexCoord3f(0,0,0); glVertex3f(8.77,5,-7.75);
        glTexCoord3f(1,0,0); glVertex3f(8.77,6,-7);
        glTexCoord3f(0,1,0); glVertex3f(8.77,7,-7);
        glTexCoord3f(1,1,0); glVertex3f(8.77,8,-7.75);
        glEnd();
        //The sign has 2 sides with swapped Normals for proper lighting
        glBegin(GL_POLYGON);
        glColor3f(1,0,0);
        glNormal3f(0,0,-0.1);
        glTexCoord3f(0,0,0); glVertex3f(9.25,8,8.76);
        glTexCoord3f(1,0,0); glVertex3f(10,7,8.76);
        glTexCoord3f(0,1,0); glVertex3f(10,6,8.76);
        glTexCoord3f(1,1,0); glVertex3f(9.25,5,8.76);
        glTexCoord3f(0,0,0); glVertex3f(7.75,5,8.76);
        glTexCoord3f(1,0,0); glVertex3f(7,6,8.76);
        glTexCoord3f(0,1,0); glVertex3f(7,7,8.76);
        glTexCoord3f(1,1,0); glVertex3f(7.75,8,8.76);
        glEnd();
        glBegin(GL_POLYGON);
        glColor3f(1,0,0);
        glNormal3f(0,0,0.1);
        glTexCoord3f(0,0,0); glVertex3f(-9.25,8,-8.76);
        glTexCoord3f(1,0,0); glVertex3f(-10,7,-8.76);
        glTexCoord3f(0,1,0); glVertex3f(-10,6,-8.76);
        glTexCoord3f(1,1,0); glVertex3f(-9.25,5,-8.76);
        glTexCoord3f(0,0,0); glVertex3f(-7.75,5,-8.76);
        glTexCoord3f(1,0,0); glVertex3f(-7,6,-8.76);
        glTexCoord3f(0,1,0); glVertex3f(-7,7,-8.76);
        glTexCoord3f(1,1,0); glVertex3f(-7.75,8,-8.76);
        glEnd();
        glBegin(GL_POLYGON);
        glColor3f(1,0,0);
        glNormal3f(0.1,0,0);
        glTexCoord3f(0,0,0); glVertex3f(-8.76,8,9.25);
        glTexCoord3f(1,0,0); glVertex3f(-8.76,7,10);
        glTexCoord3f(0,1,0); glVertex3f(-8.76,6,10);
        glTexCoord3f(1,1,0); glVertex3f(-8.76,5,9.25);
        glTexCoord3f(0,0,0); glVertex3f(-8.76,5,7.75);
        glTexCoord3f(1,0,0); glVertex3f(-8.76,6,7);
        glTexCoord3f(0,1,0); glVertex3f(-8.76,7,7);
        glTexCoord3f(1,1,0); glVertex3f(-8.76,8,7.75);
        glEnd();
        glBegin(GL_POLYGON);
        glColor3f(1,0,0);
        glNormal3f(-0.1,0,0);
        glTexCoord3f(0,0,0); glVertex3f(8.76,8,-9.25);
        glTexCoord3f(1,0,0); glVertex3f(8.76,7,-10);
        glTexCoord3f(0,1,0); glVertex3f(8.76,6,-10);
        glTexCoord3f(1,1,0); glVertex3f(8.76,5,-9.25);
        glTexCoord3f(0,0,0); glVertex3f(8.76,5,-7.75);
        glTexCoord3f(1,0,0); glVertex3f(8.76,6,-7);
        glTexCoord3f(0,1,0); glVertex3f(8.76,7,-7);
        glTexCoord3f(1,1,0); glVertex3f(8.76,8,-7.75);
        glEnd();
        glPopMatrix();
        glDisable(GL_TEXTURE_2D);
    }
}

//Reused from hw5, edited from canvas code
static void Vertex(double th, double zh)
{
    double x = Sin(th)*Cos(zh);
    double y = Cos(th)*Cos(zh);
    double z = Sin(zh);
    glNormal3d(x, y, z);
    glVertex3d(x, y, z);
}

//The represents the sun and thus has the same position as the light
//Reused from hw5, edited from canvas code
static void ball(double x, double y, double z, double r)
{
    glPushMatrix();
    glTranslated(x, y, z);
    glScaled(r, r, r);
    //White ball with yellow specular
    float yellow[] = {1.0, 1.0, 0.0, 1.0};
    float Emission[] = {0.0, 0.0, 0.01 * emission, 1.0};
    glColor3f(1, 1, 1);
    glMaterialf(GL_FRONT, GL_SHININESS, shiny);
    glMaterialfv(GL_FRONT, GL_SPECULAR, yellow);
    glMaterialfv(GL_FRONT, GL_EMISSION, Emission);
    //Bands of latitude
    for (int zh = -90; zh < 90; zh += inc)
    {
        glBegin(GL_QUAD_STRIP);
        for (int th = 0; th <= 360; th += 2 * inc)
        {
            Vertex(th, zh);
            Vertex(th, zh + inc);
        }
        glEnd();
    }
    glPopMatrix();
}

//Car code that has been expanded upon since HW3
/*Function used to draw the cars
x, y, and z control the position
r, f, and t control the dimensions of the car in the x, y, and z dimensions respectively
rad controls the wheel radius
nh controls the orientation*/
static void cars(double x, double y, double z, double f, double r, double t, double nh)
{
//These are the expected values for a standard car, and are used for the first car call in display
    //double r = 1;
    //double f = r*2;
    //double t = r*4;

    //Setting specular color to white
    float white[] = {1,1,1,1};
    float black[] = {0,0,0,1};
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shiny);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,black);

    glPushMatrix();
    //Offset
    glTranslated(x,y,z);
    glRotated(nh,0,1,0);
    //Main Car Body
    glScaled(0.1,0.1,0.1);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,texMode?GL_REPLACE:GL_MODULATE);

    //Bulding the car has been rearranged so all windows (Glass.bmp) are after the body (Metal.bmp)
    glBindTexture(GL_TEXTURE_2D,textures[0]);
    glBegin(GL_QUADS);
    //Front
    glColor3f(1,0.5,0);
    glNormal3f(0,0,+t);
    //Texture coordinates are applied for every part of the car alongside the vertex coordinates
    glTexCoord2f(0,0); glVertex3f(-f,-r,+t+1);
    glTexCoord2f(1,0); glVertex3f(+f,-r,+t+1);
    glTexCoord2f(0,1); glVertex3f(+f,+r,+t+1);
    glTexCoord2f(1,1); glVertex3f(-f,+r,+t+1);
    //Back
    glColor3f(1,0.5,0);
    glNormal3f(0,0,-t);
    glTexCoord2f(0,0); glVertex3f(+f,-r,-t-1);
    glTexCoord2f(1,0); glVertex3f(-f,-r,-t-1);
    glTexCoord2f(0,1); glVertex3f(-f,+r,-t-1);
    glTexCoord2f(1,1); glVertex3f(+f,+r,-t-1);
    //Right
    glColor3f(1,0.5,0);
    glNormal3f(+f,0,0);
    glTexCoord2f(0,0); glVertex3f(+f,-r,+t+1);
    glTexCoord2f(1,0); glVertex3f(+f,-r,-t-1);
    glTexCoord2f(0,1); glVertex3f(+f,+r,-t-1);
    glTexCoord2f(1,1); glVertex3f(+f,+r,+t+1);
    //Left
    glColor3f(1,0.5,0);
    glNormal3f(-f,0,0);
    glTexCoord2f(0,0); glVertex3f(-f,-r,-t-1);
    glTexCoord2f(1,0); glVertex3f(-f,-r,+t+1);
    glTexCoord2f(0,1); glVertex3f(-f,+r,+t+1);
    glTexCoord2f(1,1); glVertex3f(-f,+r,-t-1);
    //Lower Top Front
    glColor3f(1,0.5,0);
    glNormal3f(0,+r,0);
    glTexCoord2f(0,0); glVertex3f(-f,+r,+t+1);
    glTexCoord2f(1,0); glVertex3f(+f,+r,+t+1);
    glTexCoord2f(0,1); glVertex3f(+f,+r,+(t*3/4));
    glTexCoord2f(1,1); glVertex3f(-f,+r,+(t*3/4));
    //Lower Top Back
    glColor3f(1,0.5,0);
    glNormal3f(0,+r,0);
    glTexCoord2f(0,0); glVertex3f(+f,+r,-t-1);
    glTexCoord2f(1,0); glVertex3f(-f,+r,-t-1);
    glTexCoord2f(0,1); glVertex3f(-f,+r,-(t*3/4));
    glTexCoord2f(1,1); glVertex3f(+f,+r,-(t*3/4));
    //Upper Top
    glColor3f(1,0.5,0);
    glNormal3f(0,+(r*2.5),0);
    glTexCoord2f(0,0); glVertex3f(-f,+(r*2.5),+(t/4));
    glTexCoord2f(1,0); glVertex3f(+f,+(r*2.5),+(t/4));
    glTexCoord2f(0,1); glVertex3f(+f,+(r*2.5),-(t/4));
    glTexCoord2f(1,1); glVertex3f(-f,+(r*2.5),-(t/4));
    //Bottom
    glColor3f(1,0.5,0);
    glNormal3f(0,-r,0);
    glTexCoord2f(0,0); glVertex3f(-f,-r,-t-1);
    glTexCoord2f(1,0); glVertex3f(+f,-r,-t-1);
    glTexCoord2f(0,1); glVertex3f(+f,-r,+t+1);
    glTexCoord2f(1,1); glVertex3f(-f,-r,+t+1);
    glEnd();
    //Right Window
    //Glass texture is loaded prior to building the windows
    glBindTexture(GL_TEXTURE_2D,textures[1]);
    glBegin(GL_QUADS); 
    glColor3f(1,1,1);
    glNormal3f(+f,0,0);
    glTexCoord2f(0,0); glVertex3f(+f,+r,+(t*3/4));
    glTexCoord2f(1,0); glVertex3f(+f,+r,-(t*3/4));
    glTexCoord2f(0,1); glVertex3f(+f,+(r*2.5),-(t/4));
    glTexCoord2f(1,1); glVertex3f(+f,+(r*2.5),+(t/4));
    //Left Window
    glColor3f(1,1,1);
    glNormal3f(-f,0,0);
    glTexCoord2f(0,0); glVertex3f(-f,+r,+(t*3/4));
    glTexCoord2f(1,0); glVertex3f(-f,+r,-(t*3/4));
    glTexCoord2f(0,1); glVertex3f(-f,+(r*2.5),-(t/4));
    glTexCoord2f(1,1); glVertex3f(-f,+(r*2.5),+(t/4));
    //1/2 Windshield
    glColor3f(1,1,1);
    glNormal3f(0,+(r*1.5),0);
    glTexCoord2f(0,0); glVertex3f(+f,+r,+(t*3/4));
    glTexCoord2f(1,0); glVertex3f(-f,+r,+(t*3/4));
    glTexCoord2f(0,1); glVertex3f(-f,+(r*2.5),+(t/4));
    glTexCoord2f(1,1); glVertex3f(+f,+(r*2.5),+(t/4));
    //Rear Windshield
    glColor3f(1,1,1);
    glNormal3f(0,+(r*1.5),0);
    glTexCoord2f(0,0); glVertex3f(-f,+r,-(t*3/4));
    glTexCoord2f(1,0); glVertex3f(+f,+r,-(t*3/4));
    glTexCoord2f(0,1); glVertex3f(+f,+(r*2.5),-(t/4));
    glTexCoord2f(1,1); glVertex3f(-f,+(r*2.5),-(t/4));
    glEnd();
    //Radio Antenna & smile
    glBegin(GL_LINES);
    glColor3f(0,0,0);
    glVertex3d(+f,(r*2.5),+(t/4));
    glVertex3d(+f,(r*2.5)+1.25,0);
    glColor3f(0,0,0);
    //happy car :)
    glVertex3d(+f*0.4,r-1.5,+t+1.01);
    glVertex3d(0,r-1.75,+t+1.01);
    glColor3f(0,0,0);
    glVertex3d(-f*0.4,r-1.5,+t+1.01);
    glVertex3d(0,r-1.75,+t+1.01);
    glEnd();
    //Wheels
    double len1 = f-(f/4);
    double rad = r;
    double len2 = f+(f/6);
    float cylTex = 0;
    //The wheels are made of cylinders created by the for loops below
    //The tires use a simple rubber texture (Tire.bmp)
    glBindTexture(GL_TEXTURE_2D,textures[2]);
    glBegin(GL_QUAD_STRIP);
    //Front right tire
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
    //r and t/2 are used to offset the wheels position in terms of y and z
    //len1 & len2 determine the length of the cylinder and are positive or negative depending on the side of the car
    //Normal vectors for the wheels are 3d rather than 3f & are done in the for loops
    //cylTex is used to calculate the texture cordinate using Pi
        cylTex = i/(2*M_PI); 
        glNormal3d(+f,(rad*cos(i))-r,(rad*sin(i))+(t/2));
    //The texture coordinates are applied based on cyltext
        glTexCoord2f(cylTex,0);
        glVertex3d(+len1,(rad*cos(i))-r,(rad*sin(i))+(t/2));
        glTexCoord2f(cylTex,1);
        glVertex3d(+len2,(rad*cos(i))-r,(rad*sin(i))+(t/2));
    }
    //The for loop was redone twice to add the circles at the end of each cylindrical tire
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI);
        glNormal3d(+len2,0,0);
    //The same is done for the two for loops responsible for handling the ends of the cylinders
        glTexCoord2f(cylTex,0);
        glVertex3d(+len2,(rad*cos(i))-r,(rad*sin(i))+(t/2));
    }
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI);
        glNormal3d(-len1,0,0);
        glTexCoord2f(cylTex,1);
        glVertex3d(+len1,(rad*cos(i))-r,(rad*sin(i))+(t/2));
    }
    glEnd();
    //Front left tire
    glBegin(GL_QUAD_STRIP);
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI); 
        glNormal3d(-f,(rad*cos(i))-r,(rad*sin(i))+(t/2));
        glTexCoord2f(cylTex,0);
        glVertex3d(-len1,(rad*cos(i))-r,(rad*sin(i))+(t/2));
        glTexCoord2f(cylTex,1);
        glVertex3d(-len2,(rad*cos(i))-r,(rad*sin(i))+(t/2));
    }
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI);
        glNormal3d(-len2,0,0);
        glTexCoord2f(cylTex,0);
        glVertex3d(-len2,(rad*cos(i))-r,(rad*sin(i))+(t/2));
    }
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI);
        glNormal3d(+len1,0,0);
        glTexCoord2f(cylTex,1);
        glVertex3d(-len1,(rad*cos(i))-r,(rad*sin(i))+(t/2));
    }
    glEnd();
    //Back right tire
    glBegin(GL_QUAD_STRIP);
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI); 
        glNormal3d(+f,(rad*cos(i))-r,(rad*sin(i))-(t/2));
        glTexCoord2f(cylTex,0);
        glVertex3d(+len1,(rad*cos(i))-r,(rad*sin(i))-(t/2));
        glTexCoord2f(cylTex,1);
        glVertex3d(+len2,(rad*cos(i))-r,(rad*sin(i))-(t/2));
    }
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI);
        glNormal3d(+len2,0,0);
        glTexCoord2f(cylTex,0);
        glVertex3d(+len2,(rad*cos(i))-r,(rad*sin(i))-(t/2));
    }
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI);
        glNormal3d(-len1,0,0);
        glTexCoord2f(cylTex,1);
        glVertex3d(+len1,(rad*cos(i))-r,(rad*sin(i))-(t/2));
    }
    glEnd();
    //Back left tire
    glBegin(GL_QUAD_STRIP);
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI); 
        glNormal3d(-f,(rad*cos(i))-r,(rad*sin(i))-(t/2));
        glTexCoord2f(cylTex,0);
        glVertex3d(-len1,(rad*cos(i))-r,(rad*sin(i))-(t/2));
        glTexCoord2f(cylTex,1);
        glVertex3d(-len2,(rad*cos(i))-r,(rad*sin(i))-(t/2));
    }
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI);
        glNormal3d(-len2,0,0);
        glTexCoord2f(cylTex,0);
        glVertex3d(-len2,(rad*cos(i))-r,(rad*sin(i))-(t/2));
    }
    glColor3f(0,0,0);
    for (int i=0;i<=360;i+=5)
    {
        cylTex = i/(2*M_PI);
        glNormal3d(+len1,0,0);
        glTexCoord2f(cylTex,1);
        glVertex3d(-len1,(rad*cos(i))-r,(rad*sin(i))-(t/2));
    }
    glEnd();
    //headlights
    glBindTexture(GL_TEXTURE_2D,textures[1]);
    ball(f-0.75,0,t+0.5,0.75);
    ball(-f+0.75,0,t+0.5,0.75);
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
}

void display()
{
    //Setting the sky color to light blue
    glClearColor(0,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glLoadIdentity();
    //Orthogonal projection
    if (mode == 2)
    {
        glRotatef(zh,1,0,0);
        glRotatef(th,0,1,0);
    }
    else
    {
        //Perspective projection
        if (mode == 1)
        {
            double Ex = -2*dim*Sin(th)*Cos(zh);
            double Ey = 2*dim*Sin(zh);
            double Ez = 2*dim*Cos(th)*Cos(zh);
            gluLookAt(Ex,Ey,Ez, 0,0,0, 0,Cos(zh),0);
        }
        //FP
        else
        {
            //Adjusting the camera and the lookat functions
		    camx = 2*dim*sin((cRot*0.1));
		    camz = -2*dim*cos((cRot*0.1));
		    gluLookAt(lookx, camy, lookz, camx + lookx, camy, camz + lookz, 0, 1, 0);
        }
    }
    glShadeModel(smooth ? GL_SMOOTH : GL_FLAT);
    float Ambient[]   = {0.01*ambient ,0.01*ambient ,0.01*ambient ,1.0};
    float Diffuse[]   = {0.01*diffuse ,0.01*diffuse ,0.01*diffuse ,1.0};
    float Specular[]  = {0.01*specular,0.01*specular,0.01*specular,1.0};
//The light has a basic day night cycle (light source revolving around the world) mode and a mode where the light can be moved manually with wasd+qe.
//Light codes reused from hw6
    if (light)
    {
        if (direction == 0)
        {
            float Position[] = {15*Cos(ph),-(10*Sin(ph)),30*Cos(ph),1};
            glColor3f(1,1,1);
            ball(Position[0],Position[1],Position[2] , 1);
            //OpenGL should normalize normal vectors
            glEnable(GL_NORMALIZE);
            //Enable lighting
            glEnable(GL_LIGHTING);
            //Location of viewer for specular calculations
            glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,local);
            //glColor sets ambient and diffuse color materials
            glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
            glEnable(GL_COLOR_MATERIAL);
            //Enable light 0
            glEnable(GL_LIGHT0);
            //Set ambient, diffuse, specular components and position of light 0
            glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
            glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
            glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
            glLightfv(GL_LIGHT0,GL_POSITION,Position);
        }
        //This mode is a stationary light souce where all coordinates are controllable.
        else
        {
            float Position[] = {lightx, lighty, lightz, 1.0};
            glColor3f(1, 1, 1);
            ball(Position[0], Position[1], Position[2], 0.1);
            glEnable(GL_NORMALIZE);
            glEnable(GL_LIGHTING);
            glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, local);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            glEnable(GL_COLOR_MATERIAL);
            glEnable(GL_LIGHT0);
            glLightfv(GL_LIGHT0, GL_AMBIENT, Ambient);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, Diffuse);
            glLightfv(GL_LIGHT0, GL_SPECULAR, Specular);
            glLightfv(GL_LIGHT0, GL_POSITION, Position);
        }
    }
    else
    {
        glDisable(GL_LIGHTING);
    }
    //cSwitch determines the side of the road the car travels on
    //4 cars travels down different roads at different speeds then turn back
    if (cSwitch == 0)
    {
        cars(ch, 0.2, -0.35, 2,1,4,270);
        cars(-ch, 0.2, -9.65, 2,1,4,90);
    }
    else
    {
        cars(ch, 0.2, 0.35, 2,1,4,90);
        cars(-ch, 0.2, -10.35, 2,1,4,270);
    }
    if(cSwitch2 == 0)
    {
        cars(8.35, 0.2, ch2, 2,1,4,180);
        cars(-7.65, 0.2, ch2, 2,1,4,180);
    }
    else
    {
        cars(7.65, 0.2, ch2, 2,1,4,0);
        cars(-8.35, 0.2, ch2, 2,1,4,0);
    }
    //Building the world
    //Every object is built and scaled to be 2X2 units, allowing everything to line up properly without much issue
    //The only exception is the cars as they must fit within <1/2 of the are of the road
    //Park
    road(0,0,90,0);
    road(2,0,90,0);
    road(-2,0,90,0);
    road(0,-10,90,0);
    road(2,-10,90,0);
    road(-2,-10,90,0);
    road(4,0,0,1);
    road(-4,0,0,1);
    road(4,-2,0,0);
    road(4,-4,0,0);
    road(4,-6,0,0);
    road(4,-8,0,0);
    road(4,-10,0,1);
    road(-4,-2,0,0);
    road(-4,-4,0,0);
    road(-4,-6,0,0);
    road(-4,-8,0,0);
    road(-4,-10,0,1);
    grassLand(0,-2,3);
    grassLand(0,-4,2);
    grassLand(0,-6,3);
    pineTree(0,-6);
    grassLand(0,-8,1);
    grassLand(-2,-2,0);
    grassLand(-2,-4,1);
    grassLand(-2,-6,2);
    grassLand(-2,-8,1);
    grassLand(2,-2,2);
    grassLand(2,-4,0);
    grassLand(2,-6,1);
    grassLand(2,-8,2);
    //Surrounding area
    grassLand(0,2,3);
    subHouse(2,10,2,180);
    subHouse(-2,10,2,180);
    subHouse(0,12.5,-12,0);
    grassLand(2,-12,3);
    grassLand(-2,-12,3);
    grassLand(6,-2,3);
    subHouse(6,12.5,-4,270);
    grassLand(6,-6,3);
    subHouse(6,7.5,-8,270);
    subHouse(-6,7.5,-2,90);
    grassLand(-6,-4,3);
    subHouse(-6,12.5,-6,90);
    grassLand(-6,-8,3);
    //+Z
    road(4,2,0,0);
    road(4,4,0,1);
    road(-4,2,0,0);
    road(-4,4,0,1);
    road(2,4,90,0);
    grassLand(2,6,3);
    road(-2,4,90,0);
    grassLand(-2,6,3);
    road(0,4,90,0);
    highRise(0,1,6,180);
    road(4,6,0,0);
    road(-4,6,0,0);
    //-Z 
    road(4,-12,0,0);
    road(4,-14,0,1);
    road(-4,-12,0,0);
    road(-4,-14,0,1);
    road(2,-14,90,0);
    highRise(2,1,-16,0);
    road(4,-16,0,0);
    road(-4,-16,0,0);
    highRise(-2,1,-16,0);
    road(0,-14,90,0);
    road(-2,-14,90,0); 
    road(-4,-16,0,0);
    grassLand(0,-16,3);
    //+X
    road(6,0,90,0);
    road(8,0,0,1);
    road(8,2,0,0);
    road(8,4,0,1);
    road(8,6,0,0);
    road(6,4,90,0);
    road(8,-2,0,0);
    road(8,-4,0,0);
    road(8,-6,0,0);
    road(8,-8,0,0);
    road(8,-10,0,1);
    road(6,-10,90,0);
    road(8,-12,0,0);
    road(8,-14,0,1);
    road(8,-16,0,0);
    road(6,-14,90,0);
    highRise(6,0.75,2,270);
    highRise(6,1.25,-12,270);
    grassLand(6,6,3);
    grassLand(6,-16,3);
    //-X
    road(-6,0,90,0);
    road(-8,0,0,1);
    road(-8,2,0,0);
    road(-8,4,0,1);
    road(-8,6,0,0);
    road(-6,4,90,0);
    road(-8,-2,0,0);
    road(-8,-4,0,0);
    road(-8,-6,0,0);
    road(-8,-8,0,0);
    road(-8,-10,0,1);
    road(-6,-10,90,0);
    road(-8,-12,0,0);
    road(-8,-14,0,1);
    road(-8,-16,0,0);
    road(-6,-14,90,0);
    highRise(-6,1.25,2,90);
    highRise(-6,0.75,-12,90);
    grassLand(-6,6,3);
    grassLand(-6,-16,3);
    glDisable(GL_LIGHTING);
//Printing all necessary information to the screen
//All info is now printed regardless of mode
    glColor3f(1,1,1);
    glWindowPos2i(5,45);
    Print("Models = %s Local Viewer = %s Ball inc = %d Light Mode = %d",smooth?"Smooth":"Flat",local?"On":"Off",inc,direction);
    glWindowPos2i(5,25);
    Print("Ambient = %d  Diffuse = %d Specular = %d Emission = %d Shininess = %.0f",ambient,diffuse,specular,emission,shiny);
    glWindowPos2i(5,5);
    Print("Angle = %d,%d  Dim = %.1f FOV = %d Projection = %d, light switch = %s",th,zh,dim,fov,mode,light?"On":"Off");
//Drawing x, y, & z axis'
    glColor3f(1,1,1);
    glBegin(GL_LINES);
    glVertex3d(0,0,0);
    glVertex3d(2,0,0);
    glVertex3d(0,0,0);
    glVertex3d(0,2,0);
    glVertex3d(0,0,0);
    glVertex3d(0,0,2);
    glEnd();
//Labeling the x, y, & z axis'
    glRasterPos3d(2,0,0);
    Print("X");
    glRasterPos3d(0,2,0);
    Print("Y");
    glRasterPos3d(0,0,2);
    Print("Z");
    glFlush();
    glutSwapBuffers();
}

//Timer is used to impliment NPC movement and the day night cycle
//More npcs to be added
void timer()
{
    //If light mode is in control mode all movement stops
    if (direction == 0)
    {
        ph = (ph+1)%360;
        //When the car reaches the end of the track it reverse direction
        if (ch >= 8 || ch <= -8)
        {
            cSwitch = (cSwitch+1)%2;
        }
        if (ch2 >= 6 || ch2 <= -16)
        {
            cSwitch2 = (cSwitch2+1)%2;
        }
        if (cSwitch == 0)
        {
            ch -= 0.1;
        }
        else if (cSwitch == 1)
        {
            ch += 0.1;
        }
        if (cSwitch2 == 0)
        {
            ch2 -= 0.2;
        }
        else if (cSwitch2 == 1)
        {
            ch2 += 0.2;
        }
    }
    glutTimerFunc(50,timer,0);
    glutPostRedisplay();
}

//th & zh are changed using arrow keys to change the viewing angle
//FP controls are also handled here
void special(int key,int x,int y)
{
    if (key == GLUT_KEY_RIGHT)
    {
        if (mode != 0)
        {
            th += 5;
        }
        else
        {
            cRot += 2;
        }
    }
    else if (key == GLUT_KEY_LEFT)
    {
        if (mode != 0)
        {
            th -= 5;
        }
        else
        {
            cRot -= 2;
        }
    }
    else if (key == GLUT_KEY_DOWN)
    {
        if (mode != 0)
        {
            zh -= 5;
        }
        else
        {
            lookx = lookx-(camx*dt);
		    lookz = lookz-(camz*dt);
        }
    }
    else if (key == GLUT_KEY_UP)
    {
        if (mode != 0)
        {
            zh += 5;
        }
        else
        {
            lookx = lookx+(camx*dt); 
		    lookz = lookz+(camz*dt);
        }
    }
    th %= 360;
    zh %= 360;
    cRot %= 360;
    Project();
    glutPostRedisplay();
}

//Basic controls are the same as hw6, although unnecessary lighting modes were removed for simplicity & FP height adjustment was added
void key(unsigned char k, int x, int y) 
{
//equals sign closes the program
    if (k == '=')
    {
        exit(0);
    }
//9 resets all parameters except player position in FP & NPC position
//Player height in FP is reset
    else if (k == '9')
    {
        th = 0;
        zh = 50;
        //A different world size is utilized depending on the projection mode
        //This is meant to make it easier to get a better look more easily
        if (mode == 1 || mode == 2)
        {
            dim = 9.8;
        }
        else
        {
            dim = 3.5;
        }
        fov = 55;
        lighty = 3;
        lightx = 0;
        lightz = 0;
        inc = 10;
        emission  =   0;
        ambient   =  10;
        diffuse   =  60;
        specular  =   0;
        shininess =   0;
        shiny   =   1;
        camy = 0.5;
        dt = 0.05;
    }
    //Commans which don't utilize increasing & decreasing a value ignore capitals
    //Commands that do allow for manually increasing & decreasing a value utilize capitals to do so
    else if (k == 'm' || k == 'M')
    {
    //Mode can be 0-2
    //0 = FP, 1 = Perspective, 2 = Orthogonal
      mode = (mode+1)%3;
    //Hitting M also changes dim to allow for a good viewing angle upon a single press
        if (mode == 1)
        {
            dim = 9.8;
        }
        else if (mode == 0)
        {
            dim = 3.5;
        }
    }
    else if (k == '<')
    {
        dim -= 0.1;
    }
    else if (k == '>')
    {
        dim += 0.1;
    }
    else if (k == '-' && k>1)
    {
        fov -= 1;
    }
    else if (k == '+' && k<179)
    {
        fov += 1;
    }
    else if (k == 'n' || k == 'N')
    {
    //There are 2 different light modes, cyclic and controlled
        direction = (direction+1)%2;
    }
    //wasd + qe changes the position of the light
    else if (k == 'q' || k == 'Q')
    {
        lighty += 0.1;
    }
    else if (k == 'e' || k == 'E')
    {
        lighty -= 0.1;
    }
    else if (k == 'd' || k == 'D')
    {
        lightx += 0.1;
    }
    else if (k == 'a' || k == 'A')
    {
        lightx -= 0.1;
    }
    else if (k == 's' || k == 'S')
    {
        lightz += 0.1;
    }
    else if (k == 'w' || k == 'W')
    {
        lightz -= 0.1;
    }
    //l toggles the light
    else if (k == 'l' || k == 'L')
    {
        light = 1-light;
    }
    //t,g,b,y,h,u and their capitalized versions alter the aspects of the light
    else if (k == 't')
    {
        ambient -= 5;
    }
    else if (k == 'T')
    {
        ambient += 5;
    }
    else if (k == 'g')
    {
        diffuse -= 5;
    }
    else if (k == 'G')
    {
       diffuse += 5;
    }
    else if (k == 'b')
    {
       specular -= 5;
    }
    else if (k == 'B')
    {
        specular += 5;
    }
    else if (k == 'y')
    {
        emission -= 5;
    }
    else if (k == 'Y')
    {
       emission += 5;
    }
    else if (k== 'h' && shininess > 0)
    {
        shininess -= 1;
        shiny = pow(2.0,shininess);
    }
    else if (k =='H')
    {
        shininess += 1;
        shiny = pow(2.0,shininess);
    }
    //inc must be above 1 to avoid a crash
    else if (k == 'u' && inc > 1)
    {
        inc -= 1;
    }
    else if (k == 'U')
    {
        inc += 1;
    }
    //1 and 2 affect the smooth and local viewing modes
    else if (k == '1')
    {
        smooth = 1-smooth;
    }
    else if (k == '2')
    {
        local = 1-local;
    }
    //The texture mode is changed with the 3 key
    else if (k == '3')
    {
        texMode = 1-texMode;
    }
    //camera height in FP is now adjustable with p and P
    else if (k == 'p')
    {
        camy -= 0.1;
    }
    else if (k == 'P')
    {
        camy += 0.1;
    }
    Project();
    glutPostRedisplay();
}

//Basic reshape function reused from canvase code
void reshape(int width,int height)
{
   asp = (height>0) ? (double)width/height : 1;
   glViewport(0,0, RES*width,RES*height);
   Project(mode?fov:0,asp,dim);
}

//main code that's been edited since HW3
int main(int argc, char *argv[])
{
    srand(time(0));
    //The random values are generated in main rather than grassLand to avoid redisplay/timer creating a flicker effect
    for (int i = 0; i < 441; i++)
    {
        RNG[i] = rand()%100;
        //A random number between 0.75 - 1.25
        RNG2[i] = (rand()%(125-75+1))+75;
        RNG2[i] = RNG2[i]/100;
    }
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(600,600);
    glutCreateWindow("Nicolas_Conlin_Final");
    #ifdef USEGLEW
        //Initialize GLEW
        if (glewInit()!=GLEW_OK) Fatal("Error initializing GLEW\n");
    #endif
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSpecialFunc(special);
    glutKeyboardFunc(key);
    //timer is now called rather than idle
    timer();
    //The textures array is filled with the textures using the LoadTexBMP function
    textures[0] = LoadTexBMP("Metal.bmp");
    textures[1] = LoadTexBMP("Glass.bmp");
    textures[2] = LoadTexBMP("Tire.bmp");
    textures[3] = LoadTexBMP("Wood.bmp");
    textures[4] = LoadTexBMP("Door1.bmp");
    textures[5] = LoadTexBMP("Siding.bmp");
    textures[6] = LoadTexBMP("Tree2.bmp");
    textures[7] = LoadTexBMP("Door2.bmp");
    textures[8] = LoadTexBMP("Quartz.bmp");
    textures[9] = LoadTexBMP("Grass.bmp");
    glutMainLoop();
    return 0;
}
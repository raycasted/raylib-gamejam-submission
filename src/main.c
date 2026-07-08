#include "raylib.h"
#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif
#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>      // Emscripten library
#endif

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

// TODO: Define your custom data types here
enum Scenes{
    RAYLIB_INTRO = 0,
    TITLE = 1,
    GAME = 2
};
enum Directions{
    TOP_LEFT = 448,
    TOP_RIGHT = 256,
    BOTTOM_LEFT = 384,
    BOTTOM_RIGHT = 640,
    IDLE = 320
};
enum Keys{
    UP_TOP_RIGHT = 0,
    DOWN_BOTTOM_LEFT = 16*4,
    LEFT_TOP_LEFT = 32*4,
    RIGHT_BOTTOM_RIGHT = 48*4,
    UP_TOP_RIGHT_PRESSED = 64*4,
    DOWN_BOTTOM_LEFT_PRESSED = 80*4,
    LEFT_TOP_LEFT_PRESSED = 96*4,
    RIGHT_BOTTOM_RIGHT_PRESSED = 112*4
};
//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 720;
static const int screenHeight = 720;

static RenderTexture2D target = { 0 };  // Render texture to render our game
static int frameCounter = 0;

// TODO: Define global variables here, recommended to make them static
static enum Scenes sceneIndex = RAYLIB_INTRO;
static void UpdateDrawFrame(void);      // Update and Draw one frame
static void DrawCubert(Vector2 pos, float radius, float height, Color top, Color left, Color right); // LMAOOO GET IT BECAUSE Q*BERT + CUBE = CUBERT AHAHHAHHAHAHAHAHAH (save me)
static void DrawMap();
float raylibFade = 0.0f; // this is for the raylib logo
float seconds = 0.0f;
int secondsLoc;
// colors:
Color topPlatformColor = BLUE;
Color leftPlatformColor = WHITE;
Color rightPlatformColor = GRAY;
// TODO: add logic for player
// player
Texture2D playerTex;
Texture2D keysTex;
Rectangle playerRec;
Vector2 playerPos = (Vector2){(float)screenWidth/2 - 32, 128 - 84};

Shader shader;
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void){
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messages
#endif
    // Initialization
    InitWindow(screenWidth, screenHeight, "raylib gamejam template");
    
    // TODO: Load resources / Initialize variables at this point
    playerTex = LoadTexture("resources/plr.png");
    keysTex = LoadTexture("resources/keys.png");
    playerRec = (Rectangle){IDLE, 0, (float)playerTex.width/12, (float)playerTex.height};
    shader = LoadShader(0, TextFormat("resources/wave100.fs", GLSL_VERSION));
    secondsLoc = GetShaderLocation(shader, "seconds");
    int freqXLoc = GetShaderLocation(shader, "freqX");
    int freqYLoc = GetShaderLocation(shader, "freqY");
    int ampXLoc = GetShaderLocation(shader, "ampX");
    int ampYLoc = GetShaderLocation(shader, "ampY");
    int speedXLoc = GetShaderLocation(shader, "speedX");
    int speedYLoc = GetShaderLocation(shader, "speedY");

    float freqX = 0.0f;
    float freqY = 25.0f;
    float ampX = 0.0f;
    float ampY = 5.0f;
    float speedX = 0.0f;
    float speedY = 8.0f;

    float screenSize[2] = { (float)GetScreenWidth(), (float)GetScreenHeight() };
    SetShaderValue(shader, GetShaderLocation(shader, "size"), &screenSize, SHADER_UNIFORM_VEC2);
    SetShaderValue(shader, freqXLoc, &freqX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, freqYLoc, &freqY, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, ampXLoc, &ampX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, ampYLoc, &ampY, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, speedXLoc, &speedX, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, speedYLoc, &speedY, SHADER_UNIFORM_FLOAT);
    // Render texture to draw, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTexture(target);
    
    // TODO: Unload all loaded resources at this point

    CloseWindow();

    return 0;
}

//--------------------------------------------------------------------------------------------
// Module Functions Definition
//--------------------------------------------------------------------------------------------
// Update and draw frame
void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    frameCounter++;
    seconds += GetFrameTime();
    SetShaderValue(shader, secondsLoc, &seconds, SHADER_UNIFORM_FLOAT);
    // Draw
    //----------------------------------------------------------------------------------
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);
    DrawFPS(0, 0);
        switch (sceneIndex) {
            case RAYLIB_INTRO:
                ClearBackground(RAYWHITE);
                DrawRectangle(screenWidth/2 - 128, screenHeight/2 - 128, 256, 256, BLACK);
                DrawRectangle(screenWidth/2 - 112, screenHeight/2 - 112, 224, 224, RAYWHITE);
                DrawText("raylib", screenWidth/2 - 44, screenHeight/2 + 48, 50, BLACK);
                DrawRectangle(0, 0, screenWidth, screenHeight, (Color){0,0,0,raylibFade});
                if(frameCounter >= 120){
                    raylibFade += 5.0f;
                }
                if(raylibFade >= 255.0f || IsKeyPressed(KEY_ENTER)){
                    frameCounter = 0;
                    sceneIndex = TITLE;
                }
                break;
            case TITLE:
                ClearBackground(BLACK);
                const char* text = "PRESS ENTER";

                int fontSize = 48;
                int textWidth = MeasureText(text, fontSize);

                int textStartX = GetScreenWidth()/2 - textWidth / 2;\
                BeginShaderMode(shader);
                    DrawText(text, textStartX, screenHeight/2 + 240 + 8, fontSize, WHITE);
                EndShaderMode();
                if(IsKeyPressed(KEY_ENTER)){
                    // on click
                    frameCounter = 0;
                    sceneIndex = GAME;
                }
                break;
            case GAME:
                ClearBackground(BLACK);
                DrawMap();
                if(IsKeyPressed(KEY_RIGHT)){
                    playerPos.x += 32;
                    playerPos.y += 48;
                }
                if(IsKeyPressed(KEY_LEFT)){
                    playerPos.x -= 32;
                    playerPos.y -= 48;
                }
                if(IsKeyPressed(KEY_DOWN)){
                    playerPos.x -= 32;
                    playerPos.y += 48;
                    
                }
                if(IsKeyPressed(KEY_UP)){
                    playerPos.x += 32;
                    playerPos.y -= 48;
                }
                DrawTextureRec(playerTex, playerRec, playerPos, WHITE);
                if(IsKeyDown(KEY_RIGHT)){
                    DrawTextureRec(keysTex, (Rectangle){RIGHT_BOTTOM_RIGHT_PRESSED, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 + 64, screenHeight - 128}, WHITE);
                }else{
                    DrawTextureRec(keysTex, (Rectangle){RIGHT_BOTTOM_RIGHT, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 + 64, screenHeight - 128}, WHITE);
                }
                if(IsKeyDown(KEY_DOWN)){
                    DrawTextureRec(keysTex, (Rectangle){DOWN_BOTTOM_LEFT_PRESSED, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2, screenHeight - 128}, WHITE);
                }else{
                    DrawTextureRec(keysTex, (Rectangle){DOWN_BOTTOM_LEFT, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2, screenHeight - 128}, WHITE);
                }
                if(IsKeyDown(KEY_LEFT)){
                    DrawTextureRec(keysTex, (Rectangle){LEFT_TOP_LEFT_PRESSED, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 - 64, screenHeight - 128}, WHITE);
                }else{
                    DrawTextureRec(keysTex, (Rectangle){LEFT_TOP_LEFT, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 - 64, screenHeight - 128}, WHITE);
                }
                if(IsKeyDown(KEY_UP)){
                    DrawTextureRec(keysTex, (Rectangle){UP_TOP_RIGHT_PRESSED, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2, screenHeight - 192}, WHITE);
                }else{
                    DrawTextureRec(keysTex, (Rectangle){UP_TOP_RIGHT, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2, screenHeight - 192}, WHITE);
                }
                
                BeginShaderMode(shader);
                DrawText("CONTROLS", screenWidth/2 - 32, screenHeight/2 + 300, 24, WHITE);
                EndShaderMode();
            default:
                break;
        }

    EndTextureMode();
    BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw render texture to screen, scaled if required
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, 
            (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

        // TODO: Draw everything that requires to be drawn at this point, maybe UI?

    EndDrawing();
}
// ugly code goes here!!
static void DrawMap(){
    DrawCubert((Vector2){(float)screenWidth/2, 128}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);

    // LEFT ROW
    DrawCubert((Vector2){(float)screenWidth/2 - 32, 128 + 48}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 - 32*2, 128 + 48*2}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 - 32*3, 128 + 48*3}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 - 32*4, 128 + 48*4}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 - 32*5, 128 + 48*5}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 - 32*6, 128 + 48*6}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    
    // RIGHT ROW
    DrawCubert((Vector2){(float)screenWidth/2 + 32, 128 + 48}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 32*2, 128 + 48*2}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 32*3, 128 + 48*3}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 32*4, 128 + 48*4}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 32*5, 128 + 48*5}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 32*6, 128 + 48*6}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);

    // MIDDLE
    DrawCubert((Vector2){(float)screenWidth/2, 128 + 96}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);

    DrawCubert((Vector2){(float)screenWidth/2 - 32, 128 + 144}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 32, 128 + 144}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);

    DrawCubert((Vector2){(float)screenWidth/2, 128 + 192}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 - 64, 128 + 192}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 64, 128 + 192}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);

    DrawCubert((Vector2){(float)screenWidth/2 + 96, 128 + 240}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 32, 128 + 240}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 - 32, 128 + 240}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 - 96, 128 + 240}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);

    DrawCubert((Vector2){(float)screenWidth/2 - 128, 128 + 288}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2, 128 + 288}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 - 64, 128 + 288}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 64, 128 + 288}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);
    DrawCubert((Vector2){(float)screenWidth/2 + 128, 128 + 288}, 32, 32, topPlatformColor, leftPlatformColor, rightPlatformColor);




}
static void DrawCubert(Vector2 pos, float radius, float height, Color top, Color left, Color right)
{
    float vRadius = radius * 0.5f; // isometric squash for the top diamond

    Vector2 topPt   = { pos.x,          pos.y - vRadius };
    Vector2 rightPt = { pos.x + radius, pos.y };
    Vector2 botPt   = { pos.x,          pos.y + vRadius };
    Vector2 leftPt  = { pos.x - radius, pos.y };

    Vector2 leftDown  = { leftPt.x,  leftPt.y  + height };
    Vector2 botDown   = { botPt.x,   botPt.y   + height };
    Vector2 rightDown = { rightPt.x, rightPt.y + height };

    // Left face
    DrawTriangle(leftPt, leftDown, botDown, left);
    DrawTriangle(leftPt, botDown, botPt, left);

    // Right face
    DrawTriangle(botPt, botDown, rightDown, right);
    DrawTriangle(botPt, rightDown, rightPt, right);

    // Top face (draw last so it sits "in front")
    DrawTriangle(topPt, leftPt, botPt, top);
    DrawTriangle(topPt, botPt, rightPt, top);
}
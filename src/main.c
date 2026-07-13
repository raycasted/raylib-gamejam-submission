// please, never remind me of the coding sins i have committed, and never remind me of this file. this is an embarassment to FOSS as a whole.
#include "raylib.h"
#include "raymath.h"
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
    BOTTOM_RIGHT = 320,
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
typedef enum EnemyType{
    POSITIVE = 1,
    NEGATIVE = 0,
    EMPTY = -1
} EnemyType;
typedef struct Enemy{
    EnemyType type;
    Vector2 pos;
} Enemy;
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
static void DrawCubert(Vector2 pos, float radius, float height, Color top, Color left, Color right); // LMAOOO GET IT BECAUSE Q*BERT + CUBE = CUBERT AHAHHAHHAHAHAHAHAH (save me) (only thing thats ai coded**)
static Vector2 CheckCubertCollision(Vector2 playerPos, int type); // returns {-1, -1} on no block collision

static void UpdateEnemies();
static void DrawEnemies();
static void SpawnRandomEnemy();


static void DrawMap();
static void ResetGame(); // resets player, state, pos, enemies and randomizes color
float raylibFade = 0.0f; // this is for the raylib logo
float seconds = 0.0f;
int secondsLoc;
// colors:
Color negativeColor = BLUE;
Color positiveColor = YELLOW;
Color leftPlatformColor = WHITE;
Color rightPlatformColor = GRAY;
// TODO: add logic for player
// player
Texture2D playerTex;
Texture2D keysTex;
Texture2D logo;
Rectangle playerRec;
Vector2 playerPos = (Vector2){(float)screenWidth/2 - 32, 128 - 84};
bool canMove = true;
bool hasWon = false;
float recThickness = 0.0f;

#define BLOCK_COUNT 28
static Vector2 blockPositions[BLOCK_COUNT];
static Color blockColors[BLOCK_COUNT];
// time since game start, couldnt find a good name :(
static double gameStartTime;
#define MAX_ENEMIES 4
static Enemy enemies[MAX_ENEMIES];
Texture2D greenSlime;
Texture2D redSlime;
const Color topColorsPositive[4] = {GOLD, PINK, GREEN, BEIGE};
const Color topColorsNegative[4] = {BLUE, PURPLE, DARKGREEN, ORANGE};
Shader shader;

Sound screenClose;
Sound raylibIntro;
Sound win;
Music bgm;
Music title;
//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void){
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messages
#endif
    // Initialization
    InitWindow(screenWidth, screenHeight, "HEX3D ISO");
    InitAudioDevice();
    
    // TODO: Load resources / Initialize variables at this point
    playerTex = LoadTexture("resources/plr.png");
    keysTex = LoadTexture("resources/keys.png");
    redSlime = LoadTexture("resources/red_slime.png");
    greenSlime = LoadTexture("resources/green_slime.png");
    playerRec = (Rectangle){BOTTOM_RIGHT, 0, (float)playerTex.width/12, (float)playerTex.height};
    shader = LoadShader(0, TextFormat("resources/wave100.fs", GLSL_VERSION));
    screenClose = LoadSound("resources/screen_close_sfx.wav");
    win = LoadSound("resources/win.wav");
    bgm = LoadMusicStream("resources/bgm.wav");
    raylibIntro = LoadSound("resources/raylibsfx.wav");
    title = LoadMusicStream("resources/title.wav");
    logo = LoadTexture("resources/promotional-hexed-iso.png");
    logo.width = 600;
    logo.height = 600;

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


    int n = 0;
    blockPositions[n++] = (Vector2){(float)screenWidth/2, 128};

    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 32, 128 + 48};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 32*2, 128 + 48*2};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 32*3, 128 + 48*3};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 32*4, 128 + 48*4};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 32*5, 128 + 48*5};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 32*6, 128 + 48*6};

    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 32, 128 + 48};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 32*2, 128 + 48*2};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 32*3, 128 + 48*3};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 32*4, 128 + 48*4};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 32*5, 128 + 48*5};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 32*6, 128 + 48*6};

    blockPositions[n++] = (Vector2){(float)screenWidth/2, 128 + 96};

    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 32, 128 + 144};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 32, 128 + 144};

    blockPositions[n++] = (Vector2){(float)screenWidth/2, 128 + 192};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 64, 128 + 192};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 64, 128 + 192};

    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 96, 128 + 240};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 32, 128 + 240};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 32, 128 + 240};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 96, 128 + 240};

    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 128, 128 + 288};
    blockPositions[n++] = (Vector2){(float)screenWidth/2, 128 + 288};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 - 64, 128 + 288};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 64, 128 + 288};
    blockPositions[n++] = (Vector2){(float)screenWidth/2 + 128, 128 + 288};
    
    PlaySound(raylibIntro);
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
    UnloadTexture(playerTex);
    UnloadTexture(keysTex);
    UnloadShader(shader);
    UnloadSound(raylibIntro);
    UnloadSound(screenClose);
    UnloadSound(win);
    UnloadMusicStream(bgm);
    UnloadMusicStream(title);
    UnloadTexture(logo);
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
    if(seconds >= 1.6) seconds = 0;
    SetShaderValue(shader, secondsLoc, &seconds, SHADER_UNIFORM_FLOAT);
    // Draw
    //----------------------------------------------------------------------------------
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);
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
                    PlayMusicStream(title);
                }
                break;
            case TITLE:
                UpdateMusicStream(title);
                ClearBackground(BLACK);
                
                const char* text = "PRESS ENTER";

                int fontSize = 32;
                int textWidth = MeasureText(text, fontSize);

                int textStartX = GetScreenWidth()/2 - textWidth / 2;
                BeginShaderMode(shader);
                    DrawTexture(logo, screenWidth/2 - 300, 100, WHITE);
                    DrawText(text, textStartX, screenHeight/2 + 240 + 8, fontSize, WHITE);
                EndShaderMode();
                if(IsKeyPressed(KEY_ENTER)){
                    // on click
                    frameCounter = 0;
                    ResetGame();
                    sceneIndex = GAME;
                    PlayMusicStream(bgm);
                }
                break;
            case GAME:
                UpdateMusicStream(bgm);
                ClearBackground(BLACK);
                DrawMap();
                Vector2 currentBlock = CheckCubertCollision(playerPos, 0);
                // draw past blocks
                if(currentBlock.x != -1) DrawCubert(currentBlock, 32, 32, positiveColor, leftPlatformColor, rightPlatformColor);
                // CONTROLS
                if(IsKeyPressed(KEY_RIGHT) && canMove){
                    playerRec.x = BOTTOM_RIGHT;
                    playerPos.x += 32;
                    playerPos.y += 48;
                }
                if(IsKeyPressed(KEY_LEFT) && canMove){
                    playerRec.x = TOP_LEFT;
                    playerPos.x -= 32;
                    playerPos.y -= 48;
                }
                if(IsKeyPressed(KEY_DOWN) && canMove){
                    playerRec.x = BOTTOM_LEFT;
                    playerPos.x -= 32;
                    playerPos.y += 48;
                    
                }
                if(IsKeyPressed(KEY_UP) && canMove){
                    playerRec.x = TOP_RIGHT;
                    playerPos.x += 32;
                    playerPos.y -= 48;
                }
                DrawTextureRec(playerTex, playerRec, playerPos, WHITE);
                DrawEnemies();
                // GAME OVER
                if(CheckCubertCollision(playerPos, 0).x == -1){
                    StopMusicStream(bgm);
                    if(canMove) frameCounter = 0;
                    canMove = false;
                    if(frameCounter >= 120){
                        playerPos.y += 5;
                        if(frameCounter%20 == 0) playerRec.width *= -1;
                    }
                    if(playerPos.y > 1000){
                        recThickness += 5;
                        DrawRectangleLinesEx((Rectangle){0, 0, screenWidth, screenHeight}, recThickness, BLACK);
                        if(playerPos.y <= 1100) PlaySound(screenClose);
                    }
                    if(playerPos.y > 2000){
                        int textWidth = MeasureText("GAME OVER!", 96);
                        
                        int textStartX = GetScreenWidth()/2 - textWidth / 2;
                        BeginShaderMode(shader);
                            DrawText("GAME OVER!", textStartX, 96, 96, RED);
                        EndShaderMode();
                    }
                    if(playerPos.y > 2500){
                        const char* text = "PRESS ENTER TO TRY AGAIN!";

                        int fontSize = 42;
                        int textWidth = MeasureText(text, fontSize);

                        int textStartX = GetScreenWidth()/2 - textWidth / 2;
                        
                        DrawText(text, textStartX, screenHeight/2 + 240 + 8, fontSize, WHITE);
                        if(IsKeyPressed(KEY_ENTER)){
                            ResetGame();
                        }
                        
                    }
                }
                // else NOT gameover draw keys
                else{
                    if(IsKeyDown(KEY_RIGHT)){
                        DrawTextureRec(keysTex, (Rectangle){RIGHT_BOTTOM_RIGHT_PRESSED, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 + 64 - 28, screenHeight - 128}, WHITE);
                    }else{
                        DrawTextureRec(keysTex, (Rectangle){RIGHT_BOTTOM_RIGHT, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 + 64 - 28, screenHeight - 128}, WHITE);
                    }
                    if(IsKeyDown(KEY_DOWN)){
                        DrawTextureRec(keysTex, (Rectangle){DOWN_BOTTOM_LEFT_PRESSED, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 - 28, screenHeight - 128}, WHITE);
                    }else{
                        DrawTextureRec(keysTex, (Rectangle){DOWN_BOTTOM_LEFT, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 - 28, screenHeight - 128}, WHITE);
                    }
                    if(IsKeyDown(KEY_LEFT)){
                        DrawTextureRec(keysTex, (Rectangle){LEFT_TOP_LEFT_PRESSED, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 - 64 - 28, screenHeight - 128}, WHITE);
                    }else{
                        DrawTextureRec(keysTex, (Rectangle){LEFT_TOP_LEFT, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 - 64 - 28, screenHeight - 128}, WHITE);
                    }
                    if(IsKeyDown(KEY_UP)){
                        DrawTextureRec(keysTex, (Rectangle){UP_TOP_RIGHT_PRESSED, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 - 28, screenHeight - 192}, WHITE);
                    }else{
                        DrawTextureRec(keysTex, (Rectangle){UP_TOP_RIGHT, 0, (float)keysTex.width/8, keysTex.height}, (Vector2){(float)screenWidth/2 - 28, screenHeight - 192}, WHITE);
                    }
                    BeginShaderMode(shader);
                    DrawText("CONTROLS", screenWidth/2 - 64, screenHeight/2 + 300, 24, WHITE);
                    DrawText("POSITIVE COLOR", (float)screenWidth/2 - 320, screenHeight/2 + 300, 24, WHITE);
                    EndShaderMode();
                    DrawCubert((Vector2){(float)screenWidth/2 - 224, screenHeight - 128}, 32, 32, positiveColor, leftPlatformColor, rightPlatformColor);
                   
                    
                }
                // WIN
                if(hasWon){
                    StopMusicStream(bgm);
                    if(frameCounter%20 == 0){
                        for(int i = 0; i < BLOCK_COUNT; i++){
                            if(ColorIsEqual(WHITE, blockColors[i])) blockColors[i] = positiveColor;
                            else blockColors[i] = WHITE;
                        }
                    }
                    if(frameCounter >= 120){
                        recThickness += 2.5;
                        DrawRectangleLinesEx((Rectangle){0, 0, screenWidth, screenHeight}, recThickness, BLACK);
                        if(recThickness == 2.5){
                            PlaySound(screenClose);
                        }
                    }
                }
                // WIN
                if(recThickness >= 720 && hasWon){
                    if(recThickness <= 723) PlaySound(win);
                    int textWidth = MeasureText("YOU WON!", 96);
                        
                    int textStartX = GetScreenWidth()/2 - textWidth / 2;
                    BeginShaderMode(shader);
                        DrawText("YOU WON!", textStartX, 96, 96, GOLD);
                    EndShaderMode();
                    const char* text = "PRESS ENTER TO PLAY AGAIN!";

                    int fontSize = 42;
                    int textWidth2 = MeasureText(text, fontSize);
                    int textStartX2 = GetScreenWidth()/2 - textWidth2 / 2;
                    
                    DrawText(text, textStartX2, screenHeight/2 + 240 + 8, fontSize, WHITE);
                    if(IsKeyPressed(KEY_ENTER)){
                        ResetGame();
                    }
                }
                // WIN CHECK
                for(int i = 0; i < BLOCK_COUNT; i++){
                    if(ColorIsEqual(blockColors[i], negativeColor)){
                        break;
                    }
                    if(i == BLOCK_COUNT - 1){
                        canMove = false;
                        hasWon = true;
                    }
                }
                // ENEMY SPAWN
                if(frameCounter % 20 == 0){
                    SpawnRandomEnemy();
                }
                if(frameCounter % 20 == 0) UpdateEnemies();
                
                
                
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
    
    int n = 0;
    Vector2 block = CheckCubertCollision(playerPos, 0);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2, 128}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    // LEFT ROW
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 32, 128 + 48}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 32*2, 128 + 48*2}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 32*3, 128 + 48*3}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 32*4, 128 + 48*4}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 32*5, 128 + 48*5}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 32*6, 128 + 48*6}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    
    // RIGHT ROW
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 32, 128 + 48}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 32*2, 128 + 48*2}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 32*3, 128 + 48*3}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 32*4, 128 + 48*4}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 32*5, 128 + 48*5}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 32*6, 128 + 48*6}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);

    // MIDDLE
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2, 128 + 96}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);

    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 32, 128 + 144}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 32, 128 + 144}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);

    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2, 128 + 192}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 64, 128 + 192}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 64, 128 + 192}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);

    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 96, 128 + 240}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 32, 128 + 240}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 32, 128 + 240}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 96, 128 + 240}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);

    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 128, 128 + 288}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2, 128 + 288}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 - 64, 128 + 288}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 64, 128 + 288}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
    if(block.x == blockPositions[n].x && block.y == blockPositions[n].y) blockColors[n] = positiveColor;
    DrawCubert((Vector2){(float)screenWidth/2 + 128, 128 + 288}, 32, 32, blockColors[n++], leftPlatformColor, rightPlatformColor);
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
static Vector2 CheckCubertCollision(Vector2 playerPos, int type)
{
    int magic = 0;
    if(type == 0) magic = 84;
    else magic = 34;
    Vector2 playerBlockPos = { playerPos.x + 32, playerPos.y + magic };
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (fabsf(playerBlockPos.x - blockPositions[i].x) < 1.0f &&
            fabsf(playerBlockPos.y - blockPositions[i].y) < 1.0f) {
            return blockPositions[i];
        }
    }
    return (Vector2){-1, -1};
}
static void ResetGame(){
    int rand = GetRandomValue(0, 3);
    gameStartTime = GetTime();
    positiveColor = topColorsPositive[rand];
    negativeColor = topColorsNegative[rand];
    playerPos = (Vector2){(float)screenWidth/2 - 32, 128 - 84};
    for(int i = 0; i < BLOCK_COUNT; i++){
        blockColors[i] = negativeColor;
    }
    for(int i = 0; i < MAX_ENEMIES; i++){
        enemies[i] = (Enemy){
            .type = EMPTY,
            .pos = (Vector2){(float)screenWidth/2 - 32, -40}
        };
    }
    hasWon = false;
    playerRec.x = BOTTOM_RIGHT;
    canMove = true;
    PlayMusicStream(bgm);
    frameCounter = 0;
    recThickness = 0.0f;
}
static void UpdateEnemies(){
    for(int i = 0; i < MAX_ENEMIES; i++){
        if(enemies[i].type != EMPTY){
            // set pos y to something reasonable if its -40
            if(enemies[i].pos.y == -40){
                enemies[i].pos.y = 128 - 34;
            }
            // TODO: change enemy ai
            int rand = GetRandomValue(0, MAX_ENEMIES - 1);
            switch(rand){
                case 0:
                    enemies[i].pos.x += 32;
                    enemies[i].pos.y += 48;
                    break;
                case 1:
                    enemies[i].pos.x -= 32;
                    enemies[i].pos.y -= 48;
                    break;
                case 2:
                    enemies[i].pos.x += 32;
                    enemies[i].pos.y -= 48;
                    break;
                case 3:
                    enemies[i].pos.x -= 32;
                    enemies[i].pos.y += 48;
                    break;
                default:
                    break;
            }
            if(CheckCubertCollision(enemies[i].pos, 1).x == -1){
                enemies[i].type = EMPTY;
                enemies[i].pos = (Vector2){(float)screenWidth/2 - 32, -40};
            }
            if(CheckCollisionPointRec(playerPos, (Rectangle){enemies[i].pos.x, enemies[i].pos.y, 64, 64})){
                // TODO: collision checks
                if(enemies[i].type == POSITIVE) playerPos.x = -12312;
            }
        }
    }
}
static void DrawEnemies(){
    for(int i = 0; i < MAX_ENEMIES; i++){
        if(enemies[i].type == POSITIVE) DrawTextureV(greenSlime, enemies[i].pos, WHITE);
        if(enemies[i].type == NEGATIVE) DrawTextureV(redSlime, enemies[i].pos, WHITE);
    }
    
}
static void SpawnRandomEnemy(){
    int i = GetRandomValue(0, MAX_ENEMIES - 1);
    int type = GetRandomValue(0, 1);
    enemies[i].type = type;
}
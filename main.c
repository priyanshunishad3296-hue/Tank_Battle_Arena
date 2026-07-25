#include <stdio.h>
#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <stdbool.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define TERRAIN_POINTS 1280

typedef enum
{
    MENU,
    COLOR_SELECT,
    WEAPON_SELECT,
    PLAYING,
    GAME_OVER

} GameState;

typedef enum
{
    MISSILE,
    HEAVY_MISSILE,
    TRIPLE_SHOT,
    CLUSTER_BOMB,
    LASER

} WeaponType;

typedef struct
{
    Vector2 position;

    float angle;
    float power;

    int health;

    Color color;

    WeaponType weapons[3];

    int currentWeapon;
    int weaponCount;
    bool turn;

int movesLeft;
float moveDistance;

} Tank;

typedef struct
{
    Vector2 position;
    Vector2 velocity;

    bool active;

    float radius;

} Projectile;

GameState gameState = MENU;

Tank player1;
Tank player2;

Projectile shell;

Vector2 explosionPos;
bool explosionActive = false;
float explosionRadius = 0;

Color tankColors[] =
{
    RED,
    BLUE,
    GREEN,
    YELLOW,
    PURPLE
};

const char *colorNames[] =
{
    "RED",
    "BLUE",
    "GREEN",
    "YELLOW",
    "PURPLE"
};

int selectedColor = 0;
int selectingPlayer = 1;

const char *weaponNames[] =
{
    "MISSILE",
    "HEAVY MISSILE",
    "TRIPLE SHOT",
    "CLUSTER BOMB",
    "LASER"
};

int selectedWeapon = 0;

float GetTerrainHeight(float x)
{
    return 500
        + 45 * sinf(x * 0.009f)
        + 25 * sinf(x * 0.025f)
        + 15 * cosf(x * 0.017f);
}

void DrawSky()
{
    DrawRectangleGradientV(
        0,
        0,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SKYBLUE,
        WHITE
    );

    DrawCircle(1100,90,45,YELLOW);

    DrawCircle(250,90,28,WHITE);
    DrawCircle(285,95,35,WHITE);
    DrawCircle(320,90,28,WHITE);

    DrawCircle(700,120,25,WHITE);
    DrawCircle(730,125,32,WHITE);
    DrawCircle(760,120,25,WHITE);
}

void DrawTerrain()
{
    for(int x=0;x<SCREEN_WIDTH;x++)
    {
        float y = GetTerrainHeight(x);

        DrawLine(
            x,
            (int)y,
            x,
            SCREEN_HEIGHT,
            DARKGREEN
        );
    }
}

void DrawTank(Tank tank)
{
    Vector2 pos = tank.position;

    // Shadow
    DrawEllipse((int)pos.x, (int)(pos.y + 12), 32, 6, Fade(BLACK, 0.25f));

    // Tracks
    DrawRectangleRounded(
        (Rectangle){pos.x - 28, pos.y - 10, 56, 18},
        0.4f,
        8,
        DARKGRAY
    );

    // Wheels
    for(int i = -20; i <= 20; i += 10)
    {
        DrawCircle((int)(pos.x + i), (int)(pos.y), 5, GRAY);
        DrawCircleLines((int)(pos.x + i), (int)(pos.y), 5, BLACK);
    }

    // Tank Body
    DrawRectangleRounded(
        (Rectangle){pos.x - 22, pos.y - 22, 44, 16},
        0.3f,
        8,
        tank.color
    );

    // Highlight
    DrawRectangle(
        pos.x - 20,
        pos.y - 21,
        40,
        3,
        Fade(WHITE, 0.35f)
    );

    // Turret
    DrawCircle((int)pos.x, (int)(pos.y - 18), 10, tank.color);
    DrawCircleLines((int)pos.x, (int)(pos.y - 18), 10, BLACK);

    // Hatch
    DrawCircle((int)pos.x, (int)(pos.y - 18), 4, DARKGRAY);

    // Cannon
   Vector2 start = {pos.x, pos.y - 18};

Vector2 end =
{
    start.x + cosf(tank.angle * DEG2RAD) * 28,
    start.y + sinf(tank.angle * DEG2RAD) * 28
};

DrawLineEx(start, end, 6, DARKGRAY);

    // Cannon tip
    DrawCircleV(end, 3, BLACK);
}

void FireProjectile(Tank *tank)
{
    shell.active = true;

    shell.position.x =
        tank->position.x + 22 +
        cosf(tank->angle * DEG2RAD) * 35;

    shell.position.y =
        tank->position.y +
        sinf(tank->angle * DEG2RAD) * 35;
    shell.velocity.x =
    cosf(tank->angle * DEG2RAD) * (tank->power * 0.45f);

    shell.velocity.y =
    sinf(tank->angle * DEG2RAD) * (tank->power * 0.45f);
}

void CheckDamage()
{
    float dx = player1.position.x - explosionPos.x;
    float dy = player1.position.y - explosionPos.y;

    float d1 = sqrtf(dx * dx + dy * dy);

    dx = player2.position.x - explosionPos.x;
    dy = player2.position.y - explosionPos.y;

    float d2 = sqrtf(dx * dx + dy * dy);

    if(d1 < 50)
        player1.health -= 20;

    if(d2 < 50)
        player2.health -= 20;

    if(player1.health < 0)
        player1.health = 0;

    if(player2.health < 0)
        player2.health = 0;
}

void InitializeGame()
{
    player1.position=(Vector2){150,GetTerrainHeight(150)-20};
    player2.position=(Vector2){1050,GetTerrainHeight(1050)-20};

    player1.angle=-30;
    player2.angle=210;

    player1.power = 25;
    player2.power = 25;

    player1.health=100;
    player2.health=100;

    player1.color=RED;
    player2.color=BLUE;

    player1.weaponCount = 0;
    player2.weaponCount = 0;

    player1.currentWeapon = 0;
    player2.currentWeapon = 0;

    player1.movesLeft = 3;
    player2.movesLeft = 3;

    player1.moveDistance = 30;
    player2.moveDistance = 30;

    player1.turn = true;
    player2.turn = false;

    shell.active=false;
    shell.radius = 5;
}

int main(void)
{
    InitWindow(SCREEN_WIDTH,SCREEN_HEIGHT,"Tank Battle Arena");

    InitAudioDevice();

    SetTargetFPS(60);

    InitializeGame();

    Music introMusic = LoadMusicStream("intro.mp3");

    Sound fireSound = LoadSound("fire.wav");

    Sound explosionSound = LoadSound("explosion.wav");


PlayMusicStream(introMusic);

    while(!WindowShouldClose())

    {
         UpdateMusicStream(introMusic);

        if(player1.power < 10) player1.power = 10;
if(player1.power > 40) player1.power = 40;

if(player2.power < 10) player2.power = 10;
if(player2.power > 40) player2.power = 40;

if(player1.angle < -90) player1.angle = -90;
if(player1.angle > 0)   player1.angle = 0;

if(player2.angle < 180) player2.angle = 180;
if(player2.angle > 270) player2.angle = 270;

if(player1.position.x<0) player1.position.x=0;
if(player1.position.x>SCREEN_WIDTH-45)
    player1.position.x=SCREEN_WIDTH-45;

if(player2.position.x<0) player2.position.x=0;
if(player2.position.x>SCREEN_WIDTH-45)
    player2.position.x=SCREEN_WIDTH-45;

        BeginDrawing();

        ClearBackground(RAYWHITE);

        switch(gameState)
        {

            case MENU:

                DrawSky();

                DrawTerrain();
                DrawTank(player1);
                DrawTank(player2);

                DrawText(
                    "TANK BATTLE ARENA",
                    330,
                    120,
                    60,
                    BLACK
                );

                DrawText(
                    "Press ENTER to Start",
                    430,
                    230,
                    35,
                    DARKGRAY
                );

if(IsKeyPressed(KEY_ENTER))
{
    StopMusicStream(introMusic);
    gameState = COLOR_SELECT;
}

            break;

           case COLOR_SELECT:

    DrawSky();

    DrawTerrain();

    DrawText(
        TextFormat("PLAYER %d", selectingPlayer),
        470,
        60,
        45,
        BLACK
    );

    DrawText(
        "Choose Tank Color",
        420,
        120,
        35,
        DARKGRAY
    );

    if(IsKeyPressed(KEY_RIGHT))
    {
        selectedColor++;

        if(selectedColor>4)
            selectedColor=0;
    }

    if(IsKeyPressed(KEY_LEFT))
    {
        selectedColor--;

        if(selectedColor<0)
            selectedColor=4;
    }

    if(selectingPlayer==1)
        player1.color=tankColors[selectedColor];
    else
        player2.color=tankColors[selectedColor];

    if(selectingPlayer==1)
        DrawTank(player1);
    else
        DrawTank(player2);

    DrawText(
        colorNames[selectedColor],
        560,
        330,
        35,
        BLACK
    );

    DrawRectangle(
        560,
        380,
        150,
        50,
        tankColors[selectedColor]
    );

    DrawText(
        "< LEFT      RIGHT >",
        430,
        470,
        25,
        BLACK
    );

    DrawText(
        "ENTER = Confirm",
        470,
        520,
        25,
        DARKGRAY
    );

    if(IsKeyPressed(KEY_ENTER))
    {
        if(selectingPlayer==1)
        {
            player1.color=tankColors[selectedColor];

            selectingPlayer=2;

            selectedColor=0;
        }
        else
        {
            player2.color=tankColors[selectedColor];

            gameState=WEAPON_SELECT;
        }
    }

break;

case WEAPON_SELECT:

    DrawSky();
    DrawTerrain();

    DrawText(
        TextFormat("PLAYER %d", selectingPlayer),
        500,
        50,
        45,
        BLACK
    );

    DrawText(
        "SELECT 3 WEAPONS",
        430,
        110,
        35,
        DARKGRAY
    );

    if(IsKeyPressed(KEY_RIGHT))
    {
        selectedWeapon++;

        if(selectedWeapon>4)
            selectedWeapon=0;
    }

    if(IsKeyPressed(KEY_LEFT))
    {
        selectedWeapon--;

        if(selectedWeapon<0)
            selectedWeapon=4;
    }

    DrawText(
        weaponNames[selectedWeapon],
        470,
        250,
        40,
        BLACK
    );

    DrawText(
        "< LEFT     RIGHT >",
        440,
        330,
        25,
        DARKGRAY
    );

    DrawText(
        "ENTER = SELECT",
        450,
        380,
        25,
        BLACK
    );

    if(IsKeyPressed(KEY_ENTER))
    {
        if(selectingPlayer==1)
        {
            player1.weapons[player1.weaponCount] =
                (WeaponType)selectedWeapon;

            player1.weaponCount++;

            if(player1.weaponCount==3)
            {
                selectingPlayer = 2;
                selectedWeapon = 0;
            }
        }
        else
        {
            player2.weapons[player2.weaponCount] =
                (WeaponType)selectedWeapon;

            player2.weaponCount++;

            if(player2.weaponCount==3)
            {
                gameState=PLAYING;
            }
        }
    }

    DrawText(
        TextFormat("Selected : %d / 3",
        selectingPlayer==1 ?
        player1.weaponCount :
        player2.weaponCount),
        500,
        470,
        30,
        RED
    );

break;

case PLAYING:

    DrawSky();
    DrawTerrain();

    // Movement
   
    if(player1.turn)
{
    if(IsKeyPressed(KEY_A) && player1.movesLeft > 0)
    {
        player1.position.x -= player1.moveDistance;
        player1.movesLeft--;
    }

    if(IsKeyPressed(KEY_D) && player1.movesLeft > 0)
    {
        player1.position.x += player1.moveDistance;
        player1.movesLeft--;
    }

    if(IsKeyPressed(KEY_W))
        player1.angle -= 1.0f;

    if(IsKeyPressed(KEY_S))
        player1.angle += 1.0f;

    if(IsKeyPressed(KEY_Q))
        player1.power -= 1.0f;

    if(IsKeyPressed(KEY_E))
        player1.power += 1.0f;
}

   if(player2.turn)
{
    if(IsKeyPressed(KEY_LEFT) && player2.movesLeft > 0)
    {
        player2.position.x -= player2.moveDistance;
        player2.movesLeft--;
    }

    if(IsKeyPressed(KEY_RIGHT) && player2.movesLeft > 0)
    {
        player2.position.x += player2.moveDistance;
        player2.movesLeft--;
    }

    if(IsKeyPressed(KEY_UP))
        player2.angle -= 1.0f;

    if(IsKeyPressed(KEY_DOWN))
        player2.angle += 1.0f;

    if(IsKeyPressed(KEY_O))
        player2.power += 1.0f;

    if(IsKeyPressed(KEY_L))
        player2.power -= 1.0f;
}

    player1.position.y =
        GetTerrainHeight(player1.position.x)-20;

    player2.position.y =
        GetTerrainHeight(player2.position.x)-20;

        if(shell.active)
{
    shell.velocity.y += 0.25f;

    shell.position.x += shell.velocity.x;
    shell.position.y += shell.velocity.y;

    DrawCircleV(shell.position,
                shell.radius,
                BLACK);

    if(shell.position.y >
       GetTerrainHeight(shell.position.x))
    {
      explosionPos = shell.position;

explosionActive = true;
explosionRadius = 0;
PlaySound(explosionSound);

CheckDamage();

shell.active = false;

player1.turn = !player1.turn;
player2.turn = !player2.turn;
    }

    if(shell.position.x < 0 ||
       shell.position.x > SCREEN_WIDTH)
    {
      explosionPos = shell.position;

explosionActive = true;
explosionRadius = 0;
PlaySound(explosionSound);

CheckDamage();

shell.active = false;

player1.turn = !player1.turn;
player2.turn = !player2.turn;
    }
}

    DrawTank(player1);
    DrawTank(player2);


// ---------- PLAYER 1 HEALTH BAR ----------
DrawRectangle(
    player1.position.x - 30,
    player1.position.y - 55,
    60,
    8,
    RED
);

DrawRectangle(
    player1.position.x - 30,
    player1.position.y - 55,
    player1.health * 0.6f,
    8,
    GREEN
);

// ---------- PLAYER 2 HEALTH BAR ----------
DrawRectangle(
    player2.position.x - 30,
    player2.position.y - 55,
    60,
    8,
    RED
);

DrawRectangle(
    player2.position.x - 30,
    player2.position.y - 55,
    player2.health * 0.6f,
    8,
    GREEN
);


   if(!shell.active)
{
    if(IsKeyPressed(KEY_SPACE))
    {
        if(player1.turn)
            FireProjectile(&player1);
        else
            FireProjectile(&player2);

        PlaySound(fireSound);
    }
}
if(explosionActive)
{
    explosionRadius += 2.5f;

    DrawCircleLines(
        explosionPos.x,
        explosionPos.y,
        explosionRadius,
        ORANGE
    );

    DrawCircle(
        explosionPos.x,
        explosionPos.y,
        explosionRadius * 0.6f,
        Fade(RED,0.5f)
    );

    if(explosionRadius > 35)
    {
        explosionActive = false;
    }
}
    DrawRectangle(0,0,1280,55,LIGHTGRAY);

    DrawText(
        player1.turn ?
        "PLAYER 1 TURN" :
        "PLAYER 2 TURN",
        20,
        15,
        30,
        BLACK
    );

    DrawText(
        TextFormat("Angle : %.0f",
        player1.turn ?
        player1.angle :
        player2.angle),
        350,
        15,
        25,
        BLACK
    );

    DrawText(
        TextFormat("Power : %.0f",
        player1.turn ?
        player1.power :
        player2.power),
        600,
        15,
        25,
        BLACK
    );

    DrawText(
    TextFormat(
        "Moves : %d",
        player1.turn ?
        player1.movesLeft :
        player2.movesLeft ),
        600,
        45,
        22,
        DARKGREEN
    );

    DrawText(
    TextFormat(
        "Weapon : %s",
        player1.turn
            ? weaponNames[player1.weapons[player1.currentWeapon]]
            : weaponNames[player2.weapons[player2.currentWeapon]]
    ),
    850,
    15,
    25,
    BLACK
);

break;

            default:
            break;
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
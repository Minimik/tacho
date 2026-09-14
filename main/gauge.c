#include "gauge.h"
#include "gfx.h"
#include "display.h"
#include "storage.h"

#include <math.h>
#include <stdio.h>


#define CX          120
#define CY          120

#define START_DEG   135.0f
#define END_DEG     405.0f

#define MAX_SPEED   8000.0f


/*
 * Nadelradius.
 *
 * Muss zu deiner bisherigen Darstellung passen.
 */
#define NEEDLE_RADIUS 91.0f


/*
 * Sicherheitsrand um die alte Nadel.
 *
 * Die Nadel ist mehrere Pixel dick.
 */
#define NEEDLE_MARGIN 5


/*
 * Bereich der digitalen Geschwindigkeitsanzeige.
 *
 * Deine Anzeige beginnt ungefähr bei x=92 / y=142.
 * Wir geben etwas Reserve.
 */
#define DIGITAL_X       88
#define DIGITAL_Y       138
#define DIGITAL_WIDTH   65
#define DIGITAL_HEIGHT  23


/*
 * Letzte Position der Nadel.
 *
 * Wird benötigt, um beim nächsten Frame genau diesen
 * Bereich aus dem Hintergrund wiederherzustellen.
 */
static bool previous_needle_valid = false;

static int previous_needle_x0;
static int previous_needle_y0;
static int previous_needle_x1;
static int previous_needle_y1;


/*
 * ---------------------------------------------------------
 * Hilfsfunktionen
 * ---------------------------------------------------------
 */

static void point_on_gauge(
    float angle_deg,
    float radius,
    int *x,
    int *y
)
{
    float a = angle_deg * (float)M_PI / 180.0f;

    *x = (int)lroundf(
        CX + cosf(a) * radius
    );

    *y = (int)lroundf(
        CY + sinf(a) * radius
    );
}


static void draw_tick(
    float angle_deg,
    float r1,
    float r2,
    uint16_t color
)
{
    int x1;
    int y1;
    int x2;
    int y2;

    point_on_gauge(angle_deg, r1, &x1, &y1);
    point_on_gauge(angle_deg, r2, &x2, &y2);

    gfx_line(
        x1,
        y1,
        x2,
        y2,
        color
    );
}


/*
 * 3x5-Ziffern.
 */
static const uint8_t digits[12][5] = {
    {
        0b111,
        0b101,
        0b101,
        0b101,
        0b111
    },
    {
        0b010,
        0b110,
        0b010,
        0b010,
        0b111
    },
    {
        0b111,
        0b001,
        0b111,
        0b100,
        0b111
    },
    {
        0b111,
        0b001,
        0b111,
        0b001,
        0b111
    },
    {
        0b101,
        0b101,
        0b111,
        0b001,
        0b001
    },
    {
        0b111,
        0b100,
        0b111,
        0b001,
        0b111
    },
    {
        0b111,
        0b100,
        0b111,
        0b101,
        0b111
    },
    {
        0b111,
        0b001,
        0b010,
        0b010,
        0b010
    },
    {
        0b111,
        0b101,
        0b111,
        0b101,
        0b111
    },
    { // '9'
        0b111,
        0b101,
        0b111,
        0b001,
        0b111
    },
    { // 'x'
        0b000,
        0b000,
        0b101,
        0b010,
        0b101
    },
    { // '/'
        0b001,
        0b010,
        0b010,
        0b010,
        0b100
    },
    { //
        0b101,
        0b101,
        0b101,
        0b101,
        0b111
    }

};


static void draw_digit(
    int x,
    int y,
    int digit,
    int scale,
    uint16_t color
)
{
    if (digit < 0 || digit > 11) {
        return;
    }

    for (int row = 0; row < 5; row++) {

        for (int col = 0; col < 3; col++) {

            if (digits[digit][row] & (1 << (2 - col))) {

                for (int dy = 0; dy < scale; dy++) {

                    for (int dx = 0; dx < scale; dx++) {

                        gfx_pixel(
                            x + col * scale + dx,
                            y + row * scale + dy,
                            color
                        );
                    }
                }
            }
        }
    }
}


static void draw_number(
    int x,
    int y,
    int number,
    int scale,
    uint16_t color
)
{
    if (number < 0) {
        number = 0;
    }

    if (number > 9999) {
        number = 9999;
    }

    int thousand = number / 1000;
    int hundreds = (number / 100) % 10;
    int tens = (number / 10) % 10;
    int ones = number % 10;


    /*
     * Führende Nullen nicht anzeigen.
     */
    if (thousand > 0) {

        draw_digit(
            x,
            y,
            thousand,
            scale,
            color
        );

        x += 12;
    }

    if (thousand > 0 || hundreds > 0) {

        draw_digit(
            x,
            y,
            hundreds,
            scale,
            color
        );

        x += 12;
    }


    if (thousand > 0 || hundreds > 0 || tens > 0) {

        draw_digit(
            x,
            y,
            tens,
            scale,
            color
        );

        x += 12;
    }


    draw_digit(
        x,
        y,
        ones,
        scale,
        color
    );
}


/*
 * ---------------------------------------------------------
 * Alten Nadelbereich berechnen
 * ---------------------------------------------------------
 */

static void calculate_needle_rect(
    float speed,
    int *x0,
    int *y0,
    int *x1,
    int *y1
)
{
    if (speed < 0.0f) {
        speed = 0.0f;
    }

    if (speed > MAX_SPEED) {
        speed = MAX_SPEED;
    }


    float angle =
        START_DEG
        + (speed / MAX_SPEED)
        * (END_DEG - START_DEG);


    int ex;
    int ey;

    point_on_gauge(
        angle,
        NEEDLE_RADIUS,
        &ex,
        &ey
    );


    /*
     * Rechteck um Linie CENTER -> ENDPOINT.
     *
     * Startpunkt ist immer CX/CY.
     */
    *x0 = CX;
    *y0 = CY;

    *x1 = CX;
    *y1 = CY;


    if (ex < *x0) {
        *x0 = ex;
    }

    if (ey < *y0) {
        *y0 = ey;
    }

    if (ex > *x1) {
        *x1 = ex;
    }

    if (ey > *y1) {
        *y1 = ey;
    }


    /*
     * Rand für die Dicke der Nadel.
     */
    *x0 -= NEEDLE_MARGIN;
    *y0 -= NEEDLE_MARGIN;

    *x1 += NEEDLE_MARGIN;
    *y1 += NEEDLE_MARGIN;


    /*
     * Clipping.
     */
    if (*x0 < 0) {
        *x0 = 0;
    }

    if (*y0 < 0) {
        *y0 = 0;
    }

    if (*x1 >= GFX_WIDTH) {
        *x1 = GFX_WIDTH - 1;
    }

    if (*y1 >= GFX_HEIGHT) {
        *y1 = GFX_HEIGHT - 1;
    }
}


/*
 * ---------------------------------------------------------
 * Haupt-Renderer
 * ---------------------------------------------------------
 */

void gauge_render(float speed_kmh)
{
    /*
     * -----------------------------------------------------
     * 1. Alten dynamischen Inhalt löschen
     * -----------------------------------------------------
     *
     * NICHT den ganzen Framebuffer löschen!
     *
     * Stattdessen holen wir den alten Bereich direkt
     * aus LittleFS zurück.
     */

    if (previous_needle_valid) {

        int width =
            previous_needle_x1
            - previous_needle_x0
            + 1;

        int height =
            previous_needle_y1
            - previous_needle_y0
            + 1;


        storage_restore_rect(
            previous_needle_x0,
            previous_needle_y0,
            width,
            height
        );
    }


    /*
     * Alte digitale Geschwindigkeitsanzeige restaurieren.
     */
    storage_restore_rect(
        DIGITAL_X,
        DIGITAL_Y,
        DIGITAL_WIDTH,
        DIGITAL_HEIGHT
    );


    /*
     * -----------------------------------------------------
     * 2. Farben
     * -----------------------------------------------------
     */

    const uint16_t white =
        RGB565(255, 255, 255);

    const uint16_t red =
        RGB565(255, 0, 0);

    const uint16_t grey =
        RGB565(130, 130, 130);


    if ( true ) {
    /*
     * -----------------------------------------------------
     * 3. Äußeren Rahmen
     * -----------------------------------------------------
     */

    gfx_circle(
        CX,
        CY,
        116,
        white
    );

    gfx_circle(
        CX,
        CY,
        117,
        grey
    );


    /*
     * -----------------------------------------------------
     * 4. Skala
     * -----------------------------------------------------
     */

    for (int speed = 0; speed <= MAX_SPEED; speed += MAX_SPEED / 90 ) {

        float angle =
            START_DEG
            + ((float)speed / MAX_SPEED)
            * (END_DEG - START_DEG);


        uint16_t color =
            ( speed >= ( 2 * MAX_SPEED / 3) )
            ? red
            : white;


        draw_tick(
            angle,
            98,
            110,
            color
        );
    }


    char text[12] = "x1000 1/U";
    // snprintf(
    //     text,
    //     sizeof(text) + 1,
    //     ""
    // );


    /*
     * Einzelne Ziffern ungefähr zentrieren.
    */
    int len = 0;

    while (text[len] != '\0') {
        len++;
    }
    int x, y;

    point_on_gauge(
        270,
        50,
        &x,
        &y
    );
    x -= 4.5 * 12;
    for (int i = 0; i < len; i++)
    {   
        int digit = -1;
        switch ( text[i] )
        {
            case '0' ... '9':
                digit = text[i] - '0';
                break;
            case 'x':
                digit = 10;
                break;
            case '/':
                digit = 11;
                break;
            case 'U':
                digit = 12;
                break;
            default:
                digit = 0;
        }     
        draw_digit(
            x,
            y,
            digit, //(text[i] - '0' <= 9)? text[i] - '0' : (text[i] == 'x')? 10 : 11,
            3,
            white
        );

        x += 12;
    }


    /*
     * Kleine Teilstriche.
     */
    for (int speed = 0; speed < MAX_SPEED; speed += MAX_SPEED / 90) {

        if ((speed % 10) == 0) {
            continue;
        }

        float angle =
            START_DEG
            + ((float)speed / MAX_SPEED)
            * (END_DEG - START_DEG);


        draw_tick(
            angle,
            103,
            110,
            grey
        );
    }


    /*
     * -----------------------------------------------------
     * 5. Roter Bereich
     * -----------------------------------------------------
     */

    gfx_arc( CX, CY, 108, START_DEG + ( 3*MAX_SPEED/(4 * MAX_SPEED) ) * (END_DEG - START_DEG), END_DEG, red, 2 );


    /*
     * -----------------------------------------------------
     * 6. Zahlen auf der Skala
     * -----------------------------------------------------
     */

    for (int speed = 0; speed <= MAX_SPEED; speed += MAX_SPEED/8) {

        float angle =
            START_DEG
            + ((float)speed / MAX_SPEED)
            * (END_DEG - START_DEG);


        int x;
        int y;

        point_on_gauge(
            angle,
            88,
            &x,
            &y
        );


        /*
         * Einfache Positionierung der Zahlen.
         */
        char text[12];

        snprintf(
            text,
            sizeof(text) + 1,
            "%d",
            (speed / 1000)
        );


        /*
         * Einzelne Ziffern ungefähr zentrieren.
         */
        int len = 0;

        while (text[len] != '\0') {
            len++;
        }

        int text_width = len * 9 + (len - 1) * 3;

        x -= text_width / 2;
        y -= 7;


        for (int i = 0; i < len; i++) {

            draw_digit(
                x,
                y,
                text[i] - '0',
                3,
                white
            );

            x += 12;
        }
    }
    }

    /*
     * -----------------------------------------------------
     * 7. Aktuelle Nadel
     * -----------------------------------------------------
     */

    if (speed_kmh < 0.0f) {
        speed_kmh = 0.0f;
    }

    if (speed_kmh > MAX_SPEED) {
        speed_kmh = MAX_SPEED;
    }


    float needle_angle =
        START_DEG
        + (speed_kmh / MAX_SPEED)
        * (END_DEG - START_DEG);


    int needle_x;
    int needle_y;


    point_on_gauge(
        needle_angle,
        NEEDLE_RADIUS,
        &needle_x,
        &needle_y
    );


    uint16_t needle_color =
        (speed_kmh >= 2*MAX_SPEED/3)
        ? red
        : white;


    /*
     * Dicke Nadel.
     */
    gfx_line(
        CX - 1,
        CY - 1,
        needle_x - 1,
        needle_y - 1,
        needle_color
    );

    gfx_line(
        CX,
        CY,
        needle_x,
        needle_y,
        needle_color
    );

    gfx_line(
        CX + 1,
        CY + 1,
        needle_x + 1,
        needle_y + 1,
        needle_color
    );


    /*
     * -----------------------------------------------------
     * 8. Mittelpunkt
     * -----------------------------------------------------
     */

    gfx_fill_circle(
        CX,
        CY,
        8,
        red
    );

    gfx_fill_circle(
        CX,
        CY,
        4,
        white
    );


    /*
     * -----------------------------------------------------
     * 9. Digitale Geschwindigkeit
     * -----------------------------------------------------
     */

    int displayed_speed =
        (int)lroundf(speed_kmh);


    //int digSpeedX = ( DISPLAY_WIDTH - ( 4 * 12 ) ) / 2;
    draw_number(
        ( DISPLAY_WIDTH - ( 4 * 12 ) ) / 2,
        142,
        displayed_speed,
        3,
        white
    );


    /*
     * -----------------------------------------------------
     * 10. Position der aktuellen Nadel merken
     * -----------------------------------------------------
     *
     * Diese Position wird beim nächsten Frame aus dem
     * Hintergrund restauriert.
     */

    calculate_needle_rect(
        speed_kmh,
        &previous_needle_x0,
        &previous_needle_y0,
        &previous_needle_x1,
        &previous_needle_y1
    );

    previous_needle_valid = true;
}
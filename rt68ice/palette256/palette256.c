/* Include the AES and VDI function declarations from the Atari GEM SDK. */
#include <gem.h>

/* The palette is displayed as sixteen columns. */
#define GRID_COLUMNS 16

/* The palette is displayed as sixteen rows. */
#define GRID_ROWS 16

/* Each swatch is twenty pixels wide: 16 * 20 equals the 320-pixel screen width. */
#define SWATCH_WIDTH 20

/* Each swatch is fifteen pixels high: 16 * 15 equals the 240-pixel screen height. */
#define SWATCH_HEIGHT 15

/* Start execution at the operating-system entry point for this application. */
int main(void)
{
    /* Store the application identifier returned by AES; it is not otherwise needed here. */
    short application_id;

    /* Store the VDI workstation handle returned when the screen workstation is opened. */
    short handle;

    /* Supply device and coordinate preferences when opening the VDI workstation. */
    short work_in[11];

    /* Receive the workstation capabilities returned by VDI. */
    short work_out[57];

    /* Hold the four inclusive coordinates required by v_bar(). */
    short rectangle[4];

    /* Use one counter for VDI setup and for the 256 palette entries. */
    short colour;

    /* Register this program with AES before using its event services. */
    application_id = appl_init();

    /* Request VDI device 1, the physical screen. */
    work_in[0] = 1;

    /* Select the default VDI settings for the remaining nine device options. */
    for (colour = 1; colour < 10; colour++)
        work_in[colour] = 1;

    /* Request raster coordinates, where one coordinate unit is one pixel. */
    work_in[10] = 2;

    /* Ask VDI to allocate and initialise a virtual workstation on the screen. */
    handle = 0;

    /* Open the workstation and receive its usable handle and capability information. */
    v_opnvwk(work_in, &handle, work_out);

    /* Draw one rectangle for each of the 256 palette indices. */
    for (colour = 0; colour < GRID_COLUMNS * GRID_ROWS; colour++) {
        /* Use opaque, solid fills rather than a patterned fill style. */
        vsf_interior(handle, FIS_SOLID);

        /* Select this palette index as the fill colour. */
        vsf_color(handle, colour);

        /* Set the left edge from the column number, which is the low four bits. */
        rectangle[0] = (colour % GRID_COLUMNS) * SWATCH_WIDTH;

        /* Set the top edge from the row number, which is the upper four bits. */
        rectangle[1] = (colour / GRID_COLUMNS) * SWATCH_HEIGHT;

        /* Set the inclusive right edge one pixel before the next swatch. */
        rectangle[2] = rectangle[0] + SWATCH_WIDTH - 1;

        /* Set the inclusive bottom edge one pixel before the next swatch. */
        rectangle[3] = rectangle[1] + SWATCH_HEIGHT - 1;

        /* Fill the rectangle through VDI, which handles RT68ICE's planar layout. */
        v_bar(handle, rectangle);
    }

    /* Wait for a key so the completed chart remains visible. */
    evnt_keybd();

    /* Release the virtual workstation before this program exits. */
    v_clsvwk(handle);

    /* Tell AES that this application has finished. */
    appl_exit();

    /* Return a success status to the operating system. */
    return 0;
}

/* Include the AES and VDI function declarations from the Atari GEM SDK. */
#include <gem.h>

/* The palette is displayed as sixteen columns. */
#define GRID_COLUMNS 16

/* The palette is displayed as sixteen rows. */
#define GRID_ROWS 16

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

    /* Store the current screen width in pixels after VDI reports it. */
    short screen_width;

    /* Store the current screen height in pixels after VDI reports it. */
    short screen_height;

    /* Provide unused message storage required by evnt_multi(), even though messages are not requested. */
    short message[8];

    /* Receive the mouse X coordinate reported by the event wait. */
    short mouse_x;

    /* Receive the mouse Y coordinate reported by the event wait. */
    short mouse_y;

    /* Receive the current mouse-button state reported by the event wait. */
    short mouse_buttons;

    /* Receive the keyboard modifier state reported by the event wait. */
    short key_state;

    /* Receive the pressed key code when a keyboard event ends the wait. */
    short key_code;

    /* Receive the number of mouse clicks when a button event ends the wait. */
    short click_count;

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

    /* Convert VDI's maximum X coordinate to the number of horizontal pixels. */
    screen_width = work_out[0] + 1;

    /* Convert VDI's maximum Y coordinate to the number of vertical pixels. */
    screen_height = work_out[1] + 1;

    /* Draw one rectangle for each of the 256 palette indices. */
    for (colour = 0; colour < GRID_COLUMNS * GRID_ROWS; colour++) {
        /* Use opaque, solid fills rather than a patterned fill style. */
        vsf_interior(handle, FIS_SOLID);

        /* Select this palette index as the fill colour. */
        vsf_color(handle, colour);

        /* Set the left edge as this column's fraction of the reported screen width. */
        rectangle[0] = (colour % GRID_COLUMNS) * screen_width / GRID_COLUMNS;

        /* Set the top edge as this row's fraction of the reported screen height. */
        rectangle[1] = (colour / GRID_COLUMNS) * screen_height / GRID_ROWS;

        /* Set the right edge one pixel before the following column begins. */
        rectangle[2] = ((colour % GRID_COLUMNS + 1) * screen_width / GRID_COLUMNS) - 1;

        /* Set the bottom edge one pixel before the following row begins. */
        rectangle[3] = ((colour / GRID_COLUMNS + 1) * screen_height / GRID_ROWS) - 1;

        /* Fill the rectangle through VDI, which handles RT68ICE's planar layout. */
        v_bar(handle, rectangle);
    }

    /* Wait until either a key is pressed or the left mouse button is clicked. */
    evnt_multi(MU_KEYBD | MU_BUTTON, 1, 1, 1,
               0, 0, 0, 0, 0,
               0, 0, 0, 0, 0,
               message, 0,
               &mouse_x, &mouse_y, &mouse_buttons, &key_state, &key_code, &click_count);

    /* Release the virtual workstation before this program exits. */
    v_clsvwk(handle);

    /* Tell AES that this application has finished. */
    appl_exit();

    /* Return a success status to the operating system. */
    return 0;
}

/** Fake Edit.com Interface */

static unsigned char width, height = 0;
static unsigned char dialog_open = 1;

void draw_topmenu(void) {
  /* draw the main menu */
  bgcolor(TEXT_COLOR_LIGHT_GRAY);
  textcolor(TEXT_COLOR_BLACK);
  text_menu(0, 0, "  File  Edit  Search  View  Options  Help");
}

void draw_bottomenu(void) {
  /* DRAW THE BOTTOM MENU */
  bgcolor(TEXT_COLOR_CYAN);
  textcolor(TEXT_COLOR_BLACK);
  // why height - 1?
  text_menu(0, height - 1, "  F1=Help  Enter=Execute  Esc=Cancel  Tab=Next Field");
}

void draw_document(void) {
  /* draw the document window */
  textcolor(TEXT_COLOR_WHITE);
  bgcolor(TEXT_COLOR_DARK_BLUE);
  window_box(0, 1, width, height);

  /* Draw the window heading */
  const char* filename = " B:/MICROBE/SRC/MAIN.C ";
  unsigned char len = 21;
  unsigned char left = (width - len) >> 1;
  cputsxy(left, 1, filename);

  /* Fake it, and draw a text file */
  window_puts("int main(void) {\n");
  window_puts("    init();\n");
  window_puts("\n");
  window_puts("    Sound* sound = sound_play(SYSTEM_SOUND, 220, 0);\n");
  window_puts("    msleep(75);\n");
  window_puts("    sound_stop(sound);\n");
  window_puts("    if(controller_mode) {\n");
  window_puts("        load_splash(\"press  start{|\", get_splash_start());\n");
  window_puts("    } else {\n");
  window_puts("        load_splash(\" press  start \", get_splash_start());\n");
  window_puts("    }\n");
  window_puts("    sound = sound_play(SYSTEM_SOUND, 440, 3);\n");
  window_puts("    msleep(75);\n");
  window_puts("    sound_stop(sound);\n");
  window_puts("    sound->waveform = WAV_SAWTOOTH; // player hit\n");
  window_puts("\n");
  window_puts("    reset(true);\n");
  window_puts("\n");
  window_puts("    while(true) {\n");
  window_puts("        sound_loop();\n");
  window_puts("        uint8_t action = input();\n");
  window_puts("        switch(action) {\n");
  window_puts("            case ACTION_PAUSE: // start\n");
  window_puts("                load_splash(\"    paused    \", NULL);\n");
  window_puts("                continue;\n");
  window_puts("            case ACTION_QUIT: // quit\n");
  window_puts("                goto quit_game;\n");
  window_puts("        }\n");
  window_puts("\n");
  window_puts("        frames++;\n");
  window_puts("        if(frames > 240) frames = 0;\n");
  window_puts("\n");
  window_puts("        update();\n");
  window_puts("\n");
  window_puts("        gfx_wait_vblank(&vctx);\n");
  window_puts("        draw();\n");
  window_puts("        gfx_wait_end_vblank(&vctx);\n");
}

void draw_dialog(void) {
  /* draw the dialog */
  unsigned char len = 49; // length of "Find What: ..."
  unsigned char left = (width - len) >> 1;
  unsigned char top = 8;
  window_color(left+1, top+1, left + len + 1, top + 8 + 1, TEXT_COLOR_BLACK);
  window_color(left, top, left + len, top + 8, TEXT_COLOR_LIGHT_GRAY);
  bgcolor(TEXT_COLOR_WHITE);
  textcolor(TEXT_COLOR_BLACK);
  window_banner(left, top, 1, "Find");

  bgcolor(TEXT_COLOR_LIGHT_GRAY);
  textcolor(TEXT_COLOR_BLACK);
  window_puts("\n");
  window_puts(" Find What: [this is the end_\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9]\n");
  window_puts("\n");
  window_puts(" [ ] Match Whole Word Only\n");
  window_puts(" [ ] Match Case\n");

  bgcolor(TEXT_COLOR_DARK_GRAY);
  textcolor(TEXT_COLOR_WHITE);
  window_banner(left, top + 7, 1, "[OK]  [Cancel]  [Help]");
}

void redraw(void) {
  draw_topmenu();
  draw_document();
  if(dialog_open) draw_dialog();
  draw_bottomenu();
}

void edit(void) {
  screensize(&width, &height);

  clr_color(TEXT_COLOR_DARK_BLUE);

  redraw();

  cursor(0);
  while(1) {
    char c = cgetc();
    switch(c) {
      case KB_ESC:
        return;
        break; // exit on escape!
      case KB_F1:
        dialog_open ^= 1;
        redraw();
    }

    textcolor(TEXT_COLOR_BLACK);
    bgcolor(TEXT_COLOR_WHITE);
  }
}
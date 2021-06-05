#pragma once
#include "global.h"
#include "lcd-driver.h"
#include "lvgl.h"

#define LV_TICK_PERIOD_MS (20)

lv_disp_draw_buf_t draw_buf;
lv_disp_drv_t disp_drv;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                   lv_color_t *color_p) {

  printf("update: %d %d %d %d\n", area->x1, area->x2, area->y1, area->y2);

  halLcdUpdate(area->x1, area->x2, area->y1, area->y2, (u16*)color_p);

  lv_disp_flush_ready(disp); /* Indicate you are ready with the flushing*/
}

static void lv_tick_task(void *arg) {
  (void)arg;

  lv_tick_inc(LV_TICK_PERIOD_MS);
}

void halLvInit() {
  static int isInited = 0;
  if (isInited) {
    return;
  }
  halLcdInit();
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, lcdFB, NULL,
                        sizeof(lcdFB) / sizeof(lv_color_t));
  lv_disp_drv_init(&disp_drv);       /*Basic initialization*/
  disp_drv.flush_cb = my_disp_flush; /*Set your driver function*/
  disp_drv.draw_buf = &draw_buf;     /*Assign the buffer to the display*/
  disp_drv.hor_res = LCD_WIDTH; /*Set the horizontal resolution of the display*/
  disp_drv.ver_res = LCD_HEIGHT; /*Set the verizontal resolution of the display*/

  lv_disp_drv_register(&disp_drv);

  /* Create and start a periodic timer interrupt to call lv_tick_inc */
  const esp_timer_create_args_t periodic_timer_args = {
      .callback = &lv_tick_task, .name = "periodic_gui"};
  static esp_timer_handle_t periodic_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(
      esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

  isInited = 1;
}

void halLvRun() { lv_task_handler(); }
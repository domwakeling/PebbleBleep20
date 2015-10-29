#include <pebble.h>
#include "progress_bar.h"

/*** DEBUG SWITCH ***/
//#define DEBUGMODE									// comment out to compile normally, reinstate to compile in debug mode

/*** CONSTANTS VIA DEFINES - UNDERLYING ***/
#define MAX_ROUND 21								// highest round
#define BASE_SPEED 8.5							// speed, in km/h, in round 1
#define INCREMENT_SPEED 0.5					// speed increase, in km/h, each round
#define LAP_DISTANCE 20.0						// distance, in m, for each lap
#define KM_DISTANCE 1000.0					// number of m in a km
#define HR_IN_SECS 3600.0						// number of seconds in an hour

/*** CONSTANTS VIA DEFINES - MODES ***/
#define PRE_RUN 0										// run_stage before we start
#define RUNNING 1										// run-stage whilst running test
#define POST_RUN 2									// run-stage showing results of test

/*** CONSTANTS VIA DEFINES - ROUND & LAP LAYERS ***/
#define ROUND_HEIGHT 42							// height of the round_layer in points
#define LAP_HEIGHT 42								// height of the lap_layer in points
#define ROUND_V_OFF 8								// vertical offset of the round_layer in points, from centre
#define LAP_V_OFF 8									// vertical offset of the lap_layer in points, from centre
#define LAP_H_OFF 2									// horizontal offset for lap counters
#define LABEL_HEIGHT 20							// height in points of labels
#define LABEL_V_OFF 6								// vertical offset of the labels in points
#define ROUND_LAB_OFF_SQ 10					// horizontal "fix" for the round label, round screen
#define ROUND_LAB_OFF_RD 32					// horizontal "fix" for the round label, square screen
#define LEVEL_LAB_OFF_SQ -4					// horizontal "fix" for the level label, round screen
#define LEVEL_LAB_OFF_RD -27				// horizontal "fix" for the level label, square screen

/*** CONSTANTS VIA DEFINES - LAPS COMPLETE ***/
#define COMPLETE_V_1_OFF_RD 10			// height in points of offset from top of screen, square screen
#define COMPLETE_V_1_OFF_SQ 6				// height in points of offset from top of screen, square screen
#define COMPLETE_V_2_OFF -2					// vertical offset, in points, for second label

/*** CONSTANTS VIA DEFINES - PROGRESS COUNTER ***/
#define PROG_V_OFF_RD 30						// vertical offset of bottom of prog bar from bottom
#define PROG_V_OFF_SQ 24						// vertical offset of bottom of prog bar from bottom
#define PROG_HEIGHT 5								// height of prog bar - ODD NUMBER
#define PROG_H_MARG	10							// horizontal margin of progress bar
#define PROG_BACK GColorClear				// background colour for progress bar
#define PROG_GREY GColorLightGray		// colour for greyed-out progress bar
#define PROG_COMP GColorRed					// colour for completed progress bar
#define PROG_REFRESH 2.0						// # of times to refresh each second

/*** CONSTANTS VIA DEFINES - VO2MAX DISPLAY ***/
#define VO2_V_OFF_RD 12							// offset from bottom
#define VO2_V_OFF_SQ 7							// offset from bottom
#define VO2_V_MARG 2								// margin between two layers

/*** CONSTANTS VIA DECLARATIONS ***/
static int laps[21] = {8,8,8,9,9,10,10,10,11,11,12,12,13,13,13,14,14,15,15,15,16};

/*** GLOBAL VARIABLES ***/
int current_round, current_lap, completed_laps;
float lap_progress;
float lap_time;
int vo2max_1, vo2max_2;
int run_stage = 0;
char start_buffer[20];
char round_buffer[4];
char lap_buffer[4];
char completed_buffer[40];
char vo2max_buffer[8];

/*** "OBJECTS" ***/
static Window *window;
static TextLayer *round_layer, *lap_layer, *completed_layer_1, *completed_layer_2, *round_label, *lap_label;
static TextLayer *vo2max_layer, *vo2max_label;
static Layer *progress_layer;
AppTimer *bleep_timer, *progress_timer;
ActionBarLayer *a_b_layer;
GBitmap *square_icon, *tri_icon, *dots_icon;

/*** FUNCTION DECLARATIONS TO BE COMPLETED LATER ***/
void start_bleep_timer();
void start_progress_timer();

/*** FUNCTIONS - UTILITIES ***/

static float get_lap_time() {
	#ifdef DEBUGMODE
	return 1.0;
	#else
	if(current_round == 0) {
		return 0.0;
	} else {
		float current_speed = BASE_SPEED + (current_round - 1) * INCREMENT_SPEED;
		float current_lap_time = HR_IN_SECS * ( 1 / current_speed ) * LAP_DISTANCE / KM_DISTANCE;
		return current_lap_time;
	}
	#endif
}

static int seconds_to_ms(float secs) {
	return (int)(secs * 1000);
}

static int update_time_in_ms() {
	return (int)( 1000.0 / PROG_REFRESH );
}

static void reset_all_counters() {
	current_round = 0;
	current_lap = 0;
	completed_laps = 0;
	lap_time = 0.0;
	lap_progress = 0.0;
}

static void update_lap_and_round() {
	if(current_round == 0 || current_lap == 0) {
		current_round = 1;
		current_lap = 1;
		vibes_long_pulse();
	} else if(current_lap < laps[current_round - 1]) {
		current_lap ++;
		vibes_short_pulse();
		completed_laps ++;
	} else {
		current_lap = 1;
		current_round ++;
		completed_laps ++;
		vibes_double_pulse();
	}
	lap_progress = 0.0;
}

static void initial_text_layer_setup(Layer *window_layer) {
	
	GRect bounds = layer_get_bounds(window_layer);
	
	GRect r_l_bounds = (GRect) {
		.origin = { 0 , (bounds.size.h - ROUND_HEIGHT) / 2 + ROUND_V_OFF},
		#ifdef PBL_ROUND
		.size = { (bounds.size.w - ACTION_BAR_WIDTH/2 )/2, ROUND_HEIGHT}
		#else
		.size = { (bounds.size.w - ACTION_BAR_WIDTH )/2, ROUND_HEIGHT}
		#endif
	};
  round_layer = text_layer_create(r_l_bounds);
	text_layer_set_text_alignment(round_layer, GTextAlignmentRight);
	text_layer_set_font(round_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
	text_layer_set_background_color(round_layer, GColorClear);
	text_layer_set_text_color(round_layer, GColorRed);
  layer_add_child(window_layer, text_layer_get_layer(round_layer));
	
	GRect l_l_bounds = (GRect) {
		#ifdef PBL_ROUND
		.origin = { (bounds.size.w - ACTION_BAR_WIDTH/2) / 2 + LAP_H_OFF, (bounds.size.h - LAP_HEIGHT) / 2 + LAP_V_OFF},
		.size = { (bounds.size.w - ACTION_BAR_WIDTH/2 ) / 2 - LAP_H_OFF, LAP_HEIGHT}
		#else
		.origin = { (bounds.size.w - ACTION_BAR_WIDTH) / 2 + LAP_H_OFF, (bounds.size.h - LAP_HEIGHT) / 2 + LAP_V_OFF},
		.size = { (bounds.size.w - ACTION_BAR_WIDTH ) / 2 - LAP_H_OFF, LAP_HEIGHT}
		#endif
	};
  lap_layer = text_layer_create(l_l_bounds);
	text_layer_set_text_alignment(lap_layer, GTextAlignmentLeft);
	text_layer_set_font(lap_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
	text_layer_set_background_color(lap_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(lap_layer));
	
	GRect r_lab_bounds = (GRect) {
		#ifdef PBL_ROUND
		.origin = { r_l_bounds.origin.x + ROUND_LAB_OFF_RD, r_l_bounds.origin.y - LABEL_HEIGHT + LABEL_V_OFF},
		#else
		.origin = { r_l_bounds.origin.x + ROUND_LAB_OFF_SQ, r_l_bounds.origin.y - LABEL_HEIGHT + LABEL_V_OFF},
		#endif
		.size = {r_l_bounds.size.w, LABEL_HEIGHT}
	};
	round_label = text_layer_create(r_lab_bounds);
	text_layer_set_text_alignment(round_label, GTextAlignmentLeft);
	text_layer_set_font(round_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_background_color(round_label, GColorClear);
	text_layer_set_text_color(round_label, GColorRed);
	layer_add_child(window_layer, text_layer_get_layer(round_label));
	
	GRect l_lab_bounds = (GRect) {
		#ifdef PBL_ROUND
		.origin = { l_l_bounds.origin.x + LEVEL_LAB_OFF_RD, l_l_bounds.origin.y - LABEL_HEIGHT + LABEL_V_OFF},
		#else
		.origin = { l_l_bounds.origin.x + LEVEL_LAB_OFF_SQ, l_l_bounds.origin.y - LABEL_HEIGHT + LABEL_V_OFF},
		#endif
		.size = {l_l_bounds.size.w, LABEL_HEIGHT}
	};
	lap_label = text_layer_create(l_lab_bounds);
	text_layer_set_text_alignment(lap_label, GTextAlignmentRight);
	text_layer_set_font(lap_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_background_color(lap_label, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(lap_label));
	
	GRect c_l_bounds = (GRect) {
		#ifdef PBL_ROUND
		.origin = { 0, COMPLETE_V_1_OFF_RD},
		.size = {bounds.size.w, LABEL_HEIGHT}
		#else
		.origin = { 0, COMPLETE_V_1_OFF_SQ},
		.size = {bounds.size.w - ACTION_BAR_WIDTH, LABEL_HEIGHT}
		#endif
	};
	completed_layer_1 = text_layer_create(c_l_bounds);
	text_layer_set_text_alignment(completed_layer_1, GTextAlignmentCenter);
	text_layer_set_font(completed_layer_1, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_background_color(completed_layer_1, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(completed_layer_1));
	
	GRect c_l2_bounds = (GRect) {
		#ifdef PBL_ROUND
		.origin = { 0, COMPLETE_V_1_OFF_RD + LABEL_HEIGHT + COMPLETE_V_2_OFF},
		.size = {bounds.size.w, LABEL_HEIGHT}
		#else
		.origin = { 0, COMPLETE_V_1_OFF_SQ + LABEL_HEIGHT + COMPLETE_V_2_OFF},
		.size = {bounds.size.w - ACTION_BAR_WIDTH, LABEL_HEIGHT}
		#endif
	};
	completed_layer_2 = text_layer_create(c_l2_bounds);
	text_layer_set_text_alignment(completed_layer_2, GTextAlignmentCenter);
	text_layer_set_font(completed_layer_2, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_background_color(completed_layer_2, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(completed_layer_2));
	
	GRect v_l_bounds = (GRect) {
		#ifdef PBL_ROUND
		.origin = { 0, bounds.size.h - LABEL_HEIGHT - VO2_V_OFF_RD},
		.size = {bounds.size.w, LABEL_HEIGHT}
		#else
		.origin = { 0, bounds.size.h - LABEL_HEIGHT - VO2_V_OFF_SQ},
		.size = {bounds.size.w - ACTION_BAR_WIDTH, LABEL_HEIGHT}
		#endif
	};
	vo2max_layer = text_layer_create(v_l_bounds);
	text_layer_set_text_alignment(vo2max_layer, GTextAlignmentCenter);
	text_layer_set_font(vo2max_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_background_color(vo2max_layer, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(vo2max_layer));
	
	GRect v_lab_bounds = (GRect) {
		.origin = { 0, v_l_bounds.origin.y - LABEL_HEIGHT + VO2_V_MARG},
		#ifdef PBL_ROUND
		.size = {bounds.size.w, LABEL_HEIGHT}
		#else
		.size = {bounds.size.w - ACTION_BAR_WIDTH, LABEL_HEIGHT}
		#endif
	};
	vo2max_label = text_layer_create(v_lab_bounds);
	text_layer_set_text_alignment(vo2max_label, GTextAlignmentCenter);
	text_layer_set_font(vo2max_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_background_color(vo2max_label, GColorClear);
	layer_add_child(window_layer, text_layer_get_layer(vo2max_label));
}

static void wind_back_one_lap() {
	current_lap --;
	if(current_lap == 0) {
		if(current_round > 1) {
			current_round --;
			current_lap = laps[current_round - 1];
		} else {
			current_lap = 0;
		}
	}
}

/*** FUNCTIONS - DISPLAY ***/

static void display_level_and_lap() {
	snprintf(round_buffer, sizeof(round_buffer), "%.2d", current_round);
	snprintf(lap_buffer, sizeof(lap_buffer), "%.2d", current_lap);
	text_layer_set_text(round_layer, round_buffer);
	text_layer_set_text(lap_layer, lap_buffer);
	text_layer_set_text(round_label, "LEVEL");
	text_layer_set_text(lap_label, "LAP");
}

static void display_completed_laps() {
	snprintf(completed_buffer, sizeof(completed_buffer), "COMPLETED: %d", completed_laps);
	text_layer_set_text(completed_layer_1, "LAPS");
	text_layer_set_text(completed_layer_2, completed_buffer);
}

static void display_vo2max() {
	float vo2max = BASE_SPEED + (current_round - 1 ) * INCREMENT_SPEED;
	vo2max = (vo2max * 6.65 - 35.8) * 0.95 + 0.182;
	vo2max_1 = (int)vo2max;
	vo2max_2 = (int)(vo2max * 100 - vo2max_1 * 100);
	snprintf(vo2max_buffer, sizeof(vo2max_buffer), "%d.%d", vo2max_1, vo2max_2);
	text_layer_set_text(vo2max_layer, vo2max_buffer);
	text_layer_set_text(vo2max_label, "VO2max");
}

static void hide_completed_laps() {
	text_layer_set_text(completed_layer_1, "");
	text_layer_set_text(completed_layer_2, "");
}

static void hide_vo2max() {
	text_layer_set_text(vo2max_layer, "");
	text_layer_set_text(vo2max_label, "");
}

static void update_display_prerun() {
	display_level_and_lap();
	hide_completed_laps();
	hide_vo2max();
	action_bar_layer_set_icon_animated(a_b_layer, BUTTON_ID_SELECT, tri_icon, false);
}

static void update_display_running() {	
	display_level_and_lap();
	display_completed_laps();
	action_bar_layer_set_icon_animated(a_b_layer, BUTTON_ID_SELECT, square_icon, false);
}

static void update_display_post_run() {
	display_level_and_lap();
	display_completed_laps();
	display_vo2max();
	action_bar_layer_set_icon_animated(a_b_layer, BUTTON_ID_SELECT, dots_icon, false);
}

static void update_display() {
	if(run_stage == PRE_RUN) {
		update_display_prerun();
	} else if(run_stage == RUNNING) {
		update_display_running();
	} else {
		update_display_post_run();
	}	
}

static void progress_layer_update_proc(Layer *l, GContext *ctx) {
	
	if(run_stage == RUNNING) {
		draw_progress_bar(l, ctx, lap_progress, PROG_BACK, PROG_GREY, PROG_COMP);
	}
	
}

/*** FUNCTIONS - TIMERS & CALLBACKS ***/

static void progress_update_callback(void *data) {
	lap_progress = lap_progress + 100.0 * (float)update_time_in_ms() / (float)seconds_to_ms(get_lap_time());
	if(lap_progress > 100.0) lap_progress = 100.0;
	start_progress_timer();
	update_display();
}

void start_progress_timer() {
		progress_timer = app_timer_register(update_time_in_ms(), progress_update_callback, NULL);
};

static void bleep_timer_callback(void *data) {
	update_lap_and_round();
	update_display();
	if(run_stage == RUNNING && current_round < MAX_ROUND ) {
		start_bleep_timer();
	} else if(run_stage == RUNNING && current_round == MAX_ROUND && current_lap < laps[MAX_ROUND - 1]) {
		start_bleep_timer();
	} else {
		// reach here if we have run to the final lap without a button being pressed
		completed_laps ++;
		run_stage ++;
		update_display();
	}
}

void start_bleep_timer() {
	if(current_round <= MAX_ROUND) {
		bleep_timer = app_timer_register(seconds_to_ms(get_lap_time()), bleep_timer_callback, NULL);
	}
}

/*** FUNCTIONS - CLICK HANDLERS ***/

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  run_stage ++;
	if(run_stage > POST_RUN) run_stage = PRE_RUN;
	if(run_stage == RUNNING) {
		update_lap_and_round();
		start_bleep_timer();
		start_progress_timer();
	} else if(run_stage == PRE_RUN) {
		reset_all_counters();
	} else if(run_stage == POST_RUN) {
		app_timer_cancel(bleep_timer);
		app_timer_cancel(progress_timer);
		wind_back_one_lap();
	}
	update_display();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

/*** FUNCTIONS - STANDARD ***/

static void window_load(Window *window) {
	
  Layer *window_layer = window_get_root_layer(window);
	
	GRect bounds = layer_get_bounds(window_layer);	
	
	GRect p_bounds = {
		#ifdef PBL_ROUND
			.origin = {PROG_H_MARG + ACTION_BAR_WIDTH , bounds.size.h - PROG_V_OFF_RD - PROG_HEIGHT},
			.size = {bounds.size.w - 2* ACTION_BAR_WIDTH - 2 * PROG_H_MARG, PROG_HEIGHT} 
		#else
			.origin = {PROG_H_MARG , bounds.size.h - PROG_V_OFF_SQ - PROG_HEIGHT},
			.size = {bounds.size.w - ACTION_BAR_WIDTH - 2 * PROG_H_MARG, PROG_HEIGHT} 
		#endif
	};
	Layer *progress_layer = layer_create(p_bounds);
	layer_set_update_proc(progress_layer, progress_layer_update_proc);
	layer_add_child(window_layer, progress_layer);
	
	initial_text_layer_setup(window_layer);
	
	a_b_layer = action_bar_layer_create();
	action_bar_layer_add_to_window(a_b_layer, window);
	action_bar_layer_set_click_config_provider(a_b_layer, click_config_provider);
	
	square_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SQUARE_ICON);
	tri_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRI_ICON);
	dots_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ELLIPSIS_ICON);
	
	reset_all_counters();
}

static void window_unload(Window *window) {
	text_layer_destroy(round_layer);
	text_layer_destroy(lap_layer);
	text_layer_destroy(completed_layer_1);
	text_layer_destroy(completed_layer_2);
	text_layer_destroy(vo2max_layer);
	text_layer_destroy(vo2max_label);
	text_layer_destroy(round_label);
	text_layer_destroy(lap_label);
	layer_destroy(progress_layer);
	action_bar_layer_destroy(a_b_layer);
	gbitmap_destroy(square_icon);
	gbitmap_destroy(tri_icon);
	gbitmap_destroy(dots_icon);
}

static void init(void) {
  window = window_create();
  //window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
	.load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
	update_display();
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

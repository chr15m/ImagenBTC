#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "util.h"
#include "http.h"

#define MY_UUID { 0x4F, 0x4E, 0xEE, 0xFA, 0x16, 0xB0, 0x46, 0xFE, 0x82, 0x1E, 0xDC, 0xCF, 0xB5, 0xD2, 0x23, 0x23 }
PBL_APP_INFO(MY_UUID,
	"ImagenBTC", "Chris McCormick",
	1, 0, /* App version */
	RESOURCE_ID_IMAGE_MENU_ICON,
	APP_INFO_WATCH_FACE);

#include "build_config.h"

/***** weather stuff *****/

#define BTC_HTTP_COOKIE 1023232323
#define BTC_KEY_ADDRESS	1
#define BTC_KEY_AMOUNT 1

/***** everything else *****/

Window window;
BmpContainer background_image_container;

TextLayer info_layer;
GFont info_font;
static char info_text[] = "pebble                                  ";

TextLayer date_layer;
GFont date_font;
static char date_text[] = "                    ";

void draw_date(){
	PblTm t;
	get_time(&t);
	
	string_format_time(date_text, sizeof(date_text), "%Y-%m-%d %H:%M", &t);
	text_layer_set_text(&date_layer, date_text);
}

void draw_info(char *info) {
	strcpy(info_text, info);
	text_layer_set_text(&info_layer, info_text);
}

void clear() {
	// clear to base view
	strcpy(info_text, "pebble");
	text_layer_set_text(&info_layer, info_text);
}

void set_timer(AppContextRef ctx) {
	app_timer_send_event(ctx, 30000, 1);
}

/***** HTTP stuff *****/

void request_weather() {
	// Build the HTTP request
	DictionaryIterator *body;
	HTTPResult result = http_out_get(URL, BTC_HTTP_COOKIE, &body);
	if(result != HTTP_OK) {
		// draw_info("!HTTP_OK");
		// weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		//clear();
		#if DEBUG
		draw_info("HTTP setup failed");
		#endif
		return;
	}
	dict_write_cstring(body, BTC_KEY_ADDRESS, ADDRESS);
	// Send it.
	if(http_out_send() != HTTP_OK) {
		// draw_info("!HTTP_OK2");
		// weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		//clear();
		#if DEBUG
		draw_info("HTTP send failed");
		#endif
		return;
	}
}

void failed(int32_t cookie, int http_status, void* context) {
	if (cookie == 0) {
		#if DEBUG
		draw_info("HTTP fail 0 cookie");
		#endif
	}
	if (cookie == BTC_HTTP_COOKIE) {
		// weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		// clear();
		#if DEBUG
		draw_info("HTTP fail weather");
		#endif
	}
}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {
	#if DEBUG
	draw_info("HTTP success");
	#endif
	if(cookie != BTC_HTTP_COOKIE) {
		#if DEBUG
		draw_info("wrong cookie");
		#endif
		return;
	}
	
	Tuple* message_tuple = dict_find(received, BTC_KEY_AMOUNT);
	if (message_tuple) {
		draw_info(message_tuple->value->cstring);
	}
}

void reconnect(void* context) {
	#if DEBUG
	draw_info("reconnected");
	#endif
	request_weather();
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
	request_weather();
	// Update again every minute.
	if (cookie)
		set_timer(ctx);
}

void handle_init(AppContextRef ctx) {
	window_init(&window, "PBBL BTC");
	window_stack_push(&window, true /* Animated */);
	window_set_fullscreen(&window, true);

	resource_init_current_app(&APP_RESOURCES);
	bmp_init_container(RESOURCE_ID_IMAGE_BACKGROUND, &background_image_container);
	layer_add_child(&window.layer, &background_image_container.layer.layer);

	date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SNAP_10));
	text_layer_init(&date_layer, GRect(10, 5, 120, 20));
	text_layer_set_text_alignment(&date_layer, GTextAlignmentCenter);
	text_layer_set_text_color(&date_layer, GColorBlack);
	text_layer_set_background_color(&date_layer, GColorClear);
	text_layer_set_font(&date_layer, date_font);
	layer_add_child(&window.layer, &date_layer.layer);
	draw_date();

	info_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SNAP_10));
	text_layer_init(&info_layer, GRect(32, 146, 80, 32));
	text_layer_set_text_alignment(&info_layer, GTextAlignmentCenter);
	text_layer_set_text_color(&info_layer, GColorBlack);
	text_layer_set_background_color(&info_layer, GColorClear);
	text_layer_set_font(&info_layer, info_font);
	layer_add_child(&window.layer, &info_layer.layer);
	draw_info("pebble");

	http_register_callbacks((HTTPCallbacks){
		.failure=failed,
		.success=success,
		.reconnect=reconnect,
	}, (void*)ctx);
	
	// set up our timer to check the weather every fifteen minutes
	set_timer((AppContextRef)ctx);
	// initiate the first weather request
	request_weather();
}

void handle_deinit(AppContextRef ctx) {
	(void)ctx;

	bmp_deinit_container(&background_image_container);
	fonts_unload_custom_font(date_font);
	fonts_unload_custom_font(info_font);
}

void handle_tick(AppContextRef ctx, PebbleTickEvent *t){
	(void)t;
	(void)ctx;
	if(t->tick_time->tm_sec%10==0) {
		// layer_mark_dirty(&minute_display_layer);
		draw_date();
	}
}

void pbl_main(void *params) {
	PebbleAppHandlers handlers = {
		.init_handler = &handle_init,
		.deinit_handler = &handle_deinit,
		.tick_info = {
			.tick_handler = &handle_tick,
			.tick_units = MINUTE_UNIT
		},
		.timer_handler = handle_timer,
		.messaging_info = {
			.buffer_sizes = {
				.inbound = 124,
				.outbound = 256,
			}
		}
	};
	app_event_loop(params, &handlers);
}


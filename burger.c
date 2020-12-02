#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <SDL2/SDL.h>


#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 270
#define HASH_PRIME 1009
#define MAX_ENTITIES 256
#define array_size(x) ((int)((sizeof(x)) / (sizeof(x[0]))))


/* ENUMS */

enum EntityType {
	player,
	hotdog,
	egg,
	platform,
	ladder,
	plate,
	tablecloth,
	top_bun,
	tomato,
	meat,
	bottom_bun,
	door,
	wall,
	all
};

typedef enum {
	false, true
} bool;

enum AnimationState {
	none,
	standing,
	walking,
	jumping,
	landing,
	climbing,
	dead,
	winning
};

enum Direction {
	right, left
};


/* STRUCTS */

typedef struct {
	float x, y;
} V2;

typedef struct {
	V2 p;
	int w, h;
} Minkowski_Box;

typedef struct {
	int id;
	enum EntityType type;
	V2 normal;
	Minkowski_Box mink;
	bool new;
} Collision;

typedef struct {
	int up, down, left, right;
	int jump;
} MotionInput;

typedef struct {
	int id;
	enum EntityType type;
	enum AnimationState anim_state;
	V2 a, v, p;
	V2 hitbox;
	V2 speed;
	V2 damp;
	V2 dest;
	int w, h;
	bool on_ground;
	bool on_ladder;
	bool ladder_transition;
	bool platform_transition;
	bool movable;
	bool permeable;
	bool dead;
	float clock;
	int n_collisions;
	Collision collision[8];
	enum Direction direction;
	MotionInput motion_input;
} Entity;

typedef struct {
	char key[30];
	void *value;
} Hash_Entry;

#pragma pack(push, 1)
typedef struct {
	/* Header */
	short signature;
	int filesize;
	int reserved;
	int dataoffset;

	/* Info header */
	int infoheadersize;
	int width;
	int height;
	short planes;
	short bitsperpixel;
	int compression;
	int imagesize;
	int xpixelsperm;
	int ypixelsperm;
	int colorsused;
	int importantcolors;
} Win_BMP_Header;
#pragma pack(pop)

typedef struct {
	unsigned char *data;
	Win_BMP_Header header;
} Win_BMP;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	int w, h;
	unsigned char *data;
} Display;

typedef struct {
	int key_up;
	int key_down;
	int key_left;
	int key_right;
	int key_a;
	int key_c;
	int key_d;
	int key_q;
	int key_f;
	int key_e;
	int key_g;
	int key_l;
	int key_m;
	int key_r;
	int key_s;
	int key_ctrl;
	int key_lshift;
	int key_space;
	int key_tab;
	int key_return;
} Input;

typedef struct {
	int w, h;
	int nbytes;
	unsigned char *data;
} Bitmap;

typedef struct {
	void *buffer, *p;
	int n_elems, max_elems, elem_size;
} Memory;


/* GLOBALS */

const float frame_dt = 1.0f / 60.0f;

struct timespec start_time, end_time;
const float ns_per_s = 1000000000;
long long int target_time = (long long int)(frame_dt * ns_per_s);

Display display;

Bitmap display_bitmap;
Bitmap background_bitmap;
Bitmap chars_bitmap;

Input old_input, new_input;

Memory memory;

Memory asset_bitmaps;

Memory bitmap_buffer;

Memory entities;

int running = 1;
bool reset_npcs_state = false;
bool start_screen_state = true;
bool playing = false;
bool win = false;

Hash_Entry bitmap_table[HASH_PRIME];

Entity npc_defaults = {
	.speed.x = 400.0f,
	.speed.y = 500.0f,
	.damp.x = 0.001f,
	.damp.y = 0.9f,
	.permeable = true,
	.movable = true,
};

Entity player_defaults = {
	.speed.x = 800.0f,
	.speed.y = 1000.0f,
	.damp.x = 0.001f,
	.damp.y = 0.9f,
	.permeable = true,
	.movable = true,
};

Entity burger_defaults = {
	.speed.y = 200.0f,
	.damp.y = 0.5f,
};

Entity plate_defaults = {
	.hitbox.y = 2
};

char *player_bitmap_list[] = {
	"girl_walk_frame1",
	"girl_walk_frame2",
	"girl_back_frame1",
	"girl_back_frame2",
	"girl_jump_frame1",
	"girl_jump_frame2",
	"girl_dead_frame1",
	"girl_dead_frame2",
	"girl_pepper_frame1",
	"girl_pepper_frame2",
	"girl_win_frame1",
	"girl_win_frame2",
	NULL
};

char *hotdog_bitmap_list[] = {
	"hotdog_frame1",
	"hotdog_frame2",
	NULL
};

char *egg_bitmap_list[] = {
	"egg_legs_frame1",
	"egg_legs_frame2",
	NULL
};

char *platform_bitmap_list[] = {
	"platform_tile",
	NULL
};

char *ladder_bitmap_list[] = {
	"ladder_tile",
	NULL
};

char *plate_bitmap_list[] = {
	"plate",
	NULL
};

char *tablecloth_bitmap_list[] = {
	"tablecloth_tile",
	NULL
};

char *top_bun_bitmap_list[] = {
	"top_bun",
	NULL
};

char *tomato_bitmap_list[] = {
	"tomato",
	NULL
};

char *meat_bitmap_list[] = {
	"meat",
	NULL
};

char *bottom_bun_bitmap_list[] = {
	"bottom_bun",
	NULL
};

char *door_bitmap_list[] = {
	"door_frame1",
	"door_frame2",
	NULL
};

char *wall_bitmap_list[] = {
	NULL
};

char *background_bitmap_list[] = {
	"background",
	NULL
};

char **entity_bitmap_list[] = {
	player_bitmap_list,
	hotdog_bitmap_list,
	egg_bitmap_list,
	platform_bitmap_list,
	ladder_bitmap_list,
	plate_bitmap_list,
	tablecloth_bitmap_list,
	top_bun_bitmap_list,
	tomato_bitmap_list,
	meat_bitmap_list,
	bottom_bun_bitmap_list,
	door_bitmap_list,
	wall_bitmap_list,
	background_bitmap_list,
	NULL
};


/* FUNCTION DECLARATIONS */

void set_dimensions(Entity *, int, int);
void set_position(Entity *, float, float);
Entity *get_entity(int);
void move_burger_component(Entity *);
void add_collision(Entity*, Entity*, V2, Minkowski_Box);
Collision *search_collisions(Entity *, enum EntityType);
void off_ladder(Entity *);
void move_entity(Entity *);
Entity *add_entity(enum EntityType, float, float);


/* FUNCTION DEFINITIONS */

void hash_insert(Hash_Entry *hash_table, char *key, void *value)
{
	int h;
	char *p;

	for (h = 0, p = key; *p; ++p)
		h = *p + h + h;

	h %= HASH_PRIME;

	while (hash_table[h].value) {
		++h;

		if (h == HASH_PRIME)
			h = 0;
	}

	strcpy(hash_table[h].key, key);
	hash_table[h].value = value;
}

void *hash_lookup(Hash_Entry *hash_table, const char *keyname)
{
	if (!keyname)
		return NULL;

	int h;
	char key[30];
	strcpy(key, keyname);
	char *p;

	for (h = 0, p = key; *p; ++p)
		h = *p + h + h;

	h %= HASH_PRIME;

	if (h < 0)
		return NULL;

	while (hash_table[h].value) {
		if (strcmp(key, hash_table[h].key) == 0) {
			return hash_table[h].value;
		}

		++h;

		if (h == HASH_PRIME)
			h = 0;
	}

	return NULL;
}

void init_memory(int max_elems)
{
	memory.buffer = (void *)calloc(max_elems, 1);
	memory.p = memory.buffer;
	memory.max_elems = max_elems;
	memory.n_elems = 0;
	memory.elem_size = 1;
}

Memory reserve_memory(int max_elems, int elem_size)
{
	Memory res;
	res.buffer = memory.p;
	res.p = res.buffer;
	res.max_elems = max_elems;
	res.n_elems = 0;
	res.elem_size = elem_size;
	memory.p = (char *)memory.p + (max_elems * elem_size);
	return res;
}

void *push_memory(Memory *dest, void *src)
{
	memcpy(dest->p, src, dest->elem_size);
	void *p = dest->p;
	dest->p = (char *)dest->p + dest->elem_size;
	++dest->n_elems;
	return p;
}

void delete_memory(Memory *block, void *elem)
{
	--block->n_elems;
	void *last_elem = (char *)block->buffer + (block->n_elems * block->elem_size);
	memcpy(elem, last_elem, block->elem_size);
	block->p = last_elem;
}

void *get_memory_block(Memory block)
{
	return block.buffer;
}

Bitmap read_win_bmp(char *filename)
{
	FILE *fp;
	Win_BMP bmp;
	Bitmap bitmap;
	fp = fopen(filename, "r");
	fseek(fp, 0, SEEK_END);
	int n = ftell(fp);
	rewind(fp);
	bmp.data = (unsigned char *)malloc(n);
	fread(bmp.data, 1, n, fp);
	bmp.header = *(Win_BMP_Header *)bmp.data;
	bmp.data += bmp.header.dataoffset;
	bitmap.nbytes = bmp.header.width * bmp.header.height * 4;
	bitmap.data = (unsigned char *)malloc(bitmap.nbytes);
	unsigned char *p = bmp.data + (bmp.header.width * 4 * (bmp.header.height - 1));
	bitmap.w = bmp.header.width;
	bitmap.h = bmp.header.height;

	for (int y = 0; y < bitmap.h; ++y) {
		for (int x = 0; x < bitmap.w * 4; x += 4) {
			unsigned char *dest = bitmap.data + (y * bitmap.w * 4) + x;
			// BMP file stores colors in BGRA order,
			// SDL requires ABGR, so:
			// colors BGRA -> ABGR = positions 0123 -> 3012
			*(dest + 0) = *(p - (y * bitmap.w * 4) + x + 3);
			*(dest + 1) = *(p - (y * bitmap.w * 4) + x + 0);
			*(dest + 2) = *(p - (y * bitmap.w * 4) + x + 1);
			*(dest + 3) = *(p - (y * bitmap.w * 4) + x + 2);
		}
	}

	return bitmap;
}

void load_win_bmps()
{
	asset_bitmaps = reserve_memory(50, sizeof(Bitmap));

	for (int i = 0; entity_bitmap_list[i]; ++i) {
		for (int j = 0; entity_bitmap_list[i][j]; ++j) {
			char filepath[100] = "";
			strcat(filepath, "assets/");
			strcat(filepath, entity_bitmap_list[i][j]);
			strcat(filepath, ".bmp");
			Bitmap bitmap = read_win_bmp(filepath);
			void *p = push_memory(&asset_bitmaps, &bitmap);
			hash_insert(bitmap_table, entity_bitmap_list[i][j], p);
		}
	}
}

void clear_bitmap(Bitmap bitmap, unsigned int color)
{
	unsigned int *bitmap_p = (unsigned int *)bitmap.data;

	for (int y = 0; y < bitmap.h; ++y) {
		for (int x = 0; x < bitmap.w; ++x) {
			*bitmap_p++ = color;
		}
	}
}

void lerp_color(unsigned int src, unsigned int *dest)
{
	float a = (src & 0xff) / 255.0f;

	if (a == 0.0f) return;

	if (a < 1.0f) {
		int dest_color = *dest;
		int b = (int)((1 - a) * (dest_color >> 24 & 0xff)) + (int)(a * (src >> 24 & 0xff));
		int g = (int)((1 - a) * (dest_color >> 16 & 0xff)) + (int)(a * (src >> 16 & 0xff));
		int r = (int)((1 - a) * (dest_color >> 8 & 0xff)) + (int)(a * (src >> 8 & 0xff));
		*dest = b << 24 | g << 16 | r << 8 | (src & 0xff);
	} else {
		*dest = src;
	}
}

Bitmap flip_bitmap(Bitmap bitmap)
{
	Bitmap flipped_bitmap;
	flipped_bitmap.w = bitmap.w;
	flipped_bitmap.h = bitmap.h;
	flipped_bitmap.data = (unsigned char *)get_memory_block(bitmap_buffer);

	for (int y = 0; y < bitmap.h; ++y) {
		unsigned int *src = (unsigned int *)bitmap.data + (y * bitmap.w);
		unsigned int *dest = (unsigned int *)flipped_bitmap.data + (y * bitmap.w);

		for (int x = 0; x < bitmap.w; ++x) {
			*(dest + x) = *(src + ((bitmap.w - 1) - x));
		}
	}

	return flipped_bitmap;
}

Bitmap color_fill_bitmap(Bitmap bitmap, int color)
{
	Bitmap color_filled_bitmap;
	color_filled_bitmap.w = bitmap.w;
	color_filled_bitmap.h = bitmap.h;
	color_filled_bitmap.data = (unsigned char *)get_memory_block(bitmap_buffer);

	for (int y = 0; y < bitmap.h; ++y) {
		unsigned int *src = (unsigned int *)bitmap.data + (y * bitmap.w);
		unsigned int *dest = (unsigned int *)color_filled_bitmap.data + (y * bitmap.w);

		for (int x = 0; x < bitmap.w; ++x) {
			if (*(src + x) & 0xff) {
				*(dest + x) = color;
			}
		}
	}

	return color_filled_bitmap;
}

void draw_bitmap(
	Bitmap src_bitmap,
	Bitmap dest_bitmap,
	int dx, int dy,
	int32_t sx_off, int32_t sy_off,
	int32_t sw_off, int32_t sh_off,
	int override_color)
{
	int x1 = dx;
	int y1 = dy;
	int x2 = dx + src_bitmap.w - sx_off + sw_off;
	int y2 = dy + src_bitmap.h - sy_off + sh_off;
	int x_off = sx_off;
	int y_off = sy_off;

	if (x1 < 0) {
		x_off -= x1;
		x1 = 0;
	}

	if (y1 < 0) {
		y_off -= y1;
		y1 = 0;
	}

	if (x2 > dest_bitmap.w)
		x2 = dest_bitmap.w;

	if (y2 > dest_bitmap.h)
		y2 = dest_bitmap.h;

	unsigned int *src = (unsigned int *)src_bitmap.data + (y_off * src_bitmap.w);
	unsigned int *dest = (unsigned int *)dest_bitmap.data + (y1 * dest_bitmap.w);

	for (int y = y1; y < y2; ++y) {
		for (int x = x1; x < x2; ++x) {
			unsigned int color = *(src + (x - x1) + x_off);
			if (override_color > -1)
				color = (override_color << 8) | (color & 0xff);
			lerp_color(color, dest + x);
		}

		src += src_bitmap.w;
		dest += dest_bitmap.w;
	}
}

Bitmap scale_bitmap(Bitmap bitmap, float scale)
{
	uint32_t w = bitmap.w;
	uint32_t h = bitmap.h;

	float w_scaled = (float)w * scale;
	float h_scaled = (float)h * scale;

	float w_ratio = (float)w / w_scaled;
	float h_ratio = (float)h / h_scaled;

	Bitmap scaled_bitmap;

	scaled_bitmap.data = malloc((int)w_scaled * (int)h_scaled * 4);
	scaled_bitmap.w = (int)(w_scaled);
	scaled_bitmap.h = (int)(h_scaled);

	uint32_t* dest = (uint32_t*)scaled_bitmap.data;

	for (int y = 0; y < scaled_bitmap.h; ++y) {
		int y_stride = (int)(y * h_ratio) * bitmap.w;
		for (int x = 0; x < scaled_bitmap.w; ++x) {
			uint32_t* src = (uint32_t*)bitmap.data + y_stride + (int)(x * w_ratio);
			*(dest + x) = *src;
		}
		dest += scaled_bitmap.w;
	}

	return scaled_bitmap;
}

void draw_string(
	Bitmap dest_bitmap,
	int32_t x, int32_t y,
	const char* s,
	float scale,
	int32_t color,
	int center)
{
	float char_width = 7 * scale;
	float char_height = 10 * scale;

	if (center) {
		x += DISPLAY_WIDTH * 0.5;
		y += DISPLAY_HEIGHT * 0.5;
		x -= roundf((strlen(s) * 0.5f) * char_width);
		y -= roundf(char_height * 0.5);
	}

	Bitmap scaled_chars = scale_bitmap(chars_bitmap, scale);

	for (int i = 0; *(s + i); ++i) {
		if (*(s + i) == ' ')
			continue;

		float bitmap_index = (float)(*(s + i) - 33) * char_width + (2.0 * scale);

		draw_bitmap(
			scaled_chars, dest_bitmap,
			x + (i * char_width), y,
			bitmap_index, 0,
			-scaled_chars.w + bitmap_index + char_width, -scaled_chars.h + char_height,
			color);
	}

	free(scaled_chars.data);
}

void init_display()
{
	display.w = DISPLAY_WIDTH;
	display.h = DISPLAY_HEIGHT;
	SDL_Init(SDL_INIT_VIDEO);
	display.window = SDL_CreateWindow(
						 "Burger Girl",
						 SDL_WINDOWPOS_CENTERED,
						 SDL_WINDOWPOS_CENTERED,
						 display.w, display.h,
						 0);
	display.renderer = SDL_CreateRenderer(
						   display.window, -1, SDL_RENDERER_ACCELERATED);
	display.texture = SDL_CreateTexture(display.renderer,
						  SDL_PIXELFORMAT_RGBA8888,
						  SDL_TEXTUREACCESS_STREAMING,
						  display.w, display.h);
	SDL_SetWindowPosition(display.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_RenderSetLogicalSize(display.renderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
	//SDL_SetWindowFullscreen(display.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	SDL_SetWindowSize(display.window, DISPLAY_WIDTH * 2, DISPLAY_HEIGHT * 2);
}

void blit_display()
{
	SDL_UpdateTexture(display.texture, NULL, (void*)display.data, display.w * 4);
	SDL_RenderCopy(display.renderer, display.texture, NULL, NULL);
	SDL_RenderPresent(display.renderer);
}

void get_input()
{
	SDL_PumpEvents();
	const unsigned char *state = SDL_GetKeyboardState(NULL);
	new_input.key_up = state[SDL_SCANCODE_UP];
	new_input.key_down = state[SDL_SCANCODE_DOWN];
	new_input.key_left = state[SDL_SCANCODE_LEFT];
	new_input.key_right = state[SDL_SCANCODE_RIGHT];
	new_input.key_a = state[SDL_SCANCODE_A];
	new_input.key_c = state[SDL_SCANCODE_C];
	new_input.key_d = state[SDL_SCANCODE_D];
	new_input.key_q = state[SDL_SCANCODE_Q];
	new_input.key_f = state[SDL_SCANCODE_F];
	new_input.key_e = state[SDL_SCANCODE_E];
	new_input.key_g = state[SDL_SCANCODE_G];
	new_input.key_l = state[SDL_SCANCODE_L];
	new_input.key_m = state[SDL_SCANCODE_M];
	new_input.key_r = state[SDL_SCANCODE_R];
	new_input.key_s = state[SDL_SCANCODE_S];
	new_input.key_ctrl = state[SDL_SCANCODE_LCTRL];
	new_input.key_lshift = state[SDL_SCANCODE_LSHIFT];
	new_input.key_space = state[SDL_SCANCODE_SPACE];
	new_input.key_tab = state[SDL_SCANCODE_TAB];
	new_input.key_return = state[SDL_SCANCODE_RETURN];
}

V2 get_accel(MotionInput motion_input)
{
	V2 a = {0.0f, 0.0f};

	if (motion_input.up)
		a.y = -1.0f;

	if (motion_input.down)
		a.y = 1.0f;

	if (motion_input.right)
		a.x = 1.0f;

	if (motion_input.left)
		a.x = -1.0f;

	return a;
}

MotionInput get_player_motion_input()
{
	MotionInput motion_input = {};

	if (new_input.key_up)
		motion_input.up = 1;

	if (new_input.key_down)
		motion_input.down = 1;

	if (new_input.key_left)
		motion_input.left = 1;

	if (new_input.key_right)
		motion_input.right = 1;

	if (new_input.key_ctrl && !old_input.key_ctrl)
		motion_input.jump = 1;

	return motion_input;
}

MotionInput get_npc_motion_input(Entity *entity)
{
	MotionInput motion_input = {};
	float target_x, target_y;
	Entity *player = get_entity(0);

	if (player) {
		target_x = player->p.x;
		target_y = player->p.y;
	} else {
		target_x = rand() % DISPLAY_WIDTH;
		target_y = rand() % DISPLAY_HEIGHT;
	}

	Collision *burger_collision;
	enum EntityType burger_search[4] = {top_bun, tomato, meat, bottom_bun};

	for (int i = 0; i < 4; ++i) {
		burger_collision = search_collisions(entity, burger_search[i]);

		if (burger_collision && burger_collision->normal.x != 0.0f) {
			motion_input.jump = 1;
			break;
		}
	}

	if ((search_collisions(entity, platform) &&
		 (search_collisions(entity, ladder) ||
		  search_collisions(entity, wall))) ||
		(entity->a.y == 0.0 && entity->a.x == 0.0)) {
		float x_diff = entity->p.x - target_x;
		float y_diff = entity->p.y - target_y;

		if (y_diff < 50) {
			motion_input.down = 1;
		} else if (y_diff > 50) {
			motion_input.up = 1;
		}

		if (x_diff < 0) {
			motion_input.right = 1;
		} else if (x_diff > 0) {
			motion_input.left = 1;
		}
	} else {
		motion_input.left = entity->motion_input.left;
		motion_input.right = entity->motion_input.right;
		motion_input.up = entity->motion_input.up;
		motion_input.down = entity->motion_input.down;
	}

	return motion_input;
}

void process_ui_input()
{
	if (new_input.key_q) {
		running = 0;
	}

	if (new_input.key_f && !old_input.key_f) {
		clear_bitmap(display_bitmap, 0);
		blit_display();
		unsigned int fs = SDL_GetWindowFlags(display.window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
		SDL_SetWindowFullscreen(display.window, fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
}

void vector_set(V2 *v, float x, float y)
{
	v->x = x;
	v->y = y;
}

float vector_dot_product(V2 v1, V2 v2)
{
	float s = (v1.x * v2.x) + (v1.y * v2.y);

	return s;
}

V2 vector_add(V2 v1, V2 v2)
{
	v1.x += v2.x;
	v1.y += v2.y;

	return v1;
}

V2 vector_subtract(V2 v1, V2 v2)
{
	v1.x -= v2.x;
	v1.y -= v2.y;

	return v1;
}

V2 vector_scalar_multiply(V2 v, float s)
{
	v.x *= s;
	v.y *= s;

	return v;
}

V2 vector_scalar_divide(V2 v, float s)
{
	v.x /= s;
	v.y /= s;

	return v;
}

V2 vector_normalize(V2 v)
{
	float lsq = vector_dot_product(v, v);

	if (lsq > 1.0f) {
		float l = sqrt(lsq);
		v = vector_scalar_divide(v, l);
	}

	return v;
}

float square(float x)
{
	return x * x;
}

V2 find_normal(Minkowski_Box mink)
{
	V2 normal = {};
	float sides[4][3] = {
		{fabs(mink.p.y + mink.h), 0.0f, -1.0f},
		{fabs(mink.p.x + mink.w), -1.0f, 0.0f},
		{fabs(mink.p.x), 1.0f, 0.0f},
		{fabs(mink.p.y), 0.0f, 1.0f}
	};
	float min = 5.0f;

	for (int i = 0; i < 4; ++i) {
		if (sides[i][0] < min) {
			min = sides[i][0];
			normal.x = sides[i][1];
			normal.y = sides[i][2];
		}
	}

	return normal;
}

void resolve_collision(Entity* oe, Entity* e1, V2* dt)
{
	for (int i = 0; i < e1->n_collisions; ++i) {
		if (!get_entity(e1->collision[i].id)->permeable) {
			V2 normal = e1->collision[i].normal;

			if (normal.y != 0.0f) {
				e1->p.y = oe->p.y;
			} else if (normal.x != 0.0f) {
				e1->p.x = oe->p.x;
			} else {
				e1->p = oe->p;
			}
		}
	}

	for (int i = 0; i < e1->n_collisions; ++i) {
		if (!get_entity(e1->collision[i].id)->permeable) {
			Entity *e2 = get_entity(e1->collision[i].id);
			V2 normal = e1->collision[i].normal;
			float t = 1.0f;

			float x1 = e1->p.x, y1 = e1->p.y;
			float x2 = e2->p.x, y2 = e2->p.y;
			int w1 = e1->hitbox.x > 0 ? e1->hitbox.x : e1->w;
			int w2 = e2->hitbox.x > 0 ? e2->hitbox.x : e2->w;
			int h1 = e1->hitbox.y > 0 ? e1->hitbox.y : e1->h;
			int h2 = e2->hitbox.y > 0 ? e2->hitbox.y : e2->h;


			if (normal.x == 1.0f && dt->x != 0.0f) {
				t = ((x1 - (w1 * 0.5f)) - (x2 + (w2 * 0.5f))) / fabs(dt->x);
			} else if (normal.x == -1.0f && dt->x != 0.0f) {
				t = ((x2 - (w2 * 0.5f)) - (x1 + (w1 * 0.5f))) / fabs(dt->x);
			} else if (normal.y == 1.0f && dt->y != 0.0f) {
				t = ((y1 - (h1 * 0.5f)) - (y2 + (h2 * 0.5f))) / fabs(dt->y);
			} else if (normal.y == -1.0f && dt->y != 0.0f) {
				t = ((y2 - (h2 * 0.5f)) - (y1 + (h1 * 0.5f))) / fabs(dt->y);
			}

			int normal_seen = 0;

			for (int j = 0; j < i; ++j) {
				V2 earlier_normal = e1->collision[j].normal;

				if (earlier_normal.x == normal.x && earlier_normal.y == normal.y) {
					normal_seen = 1;
				}
			}

			if (!normal_seen) {
				e1->p = vector_add(e1->p, vector_scalar_multiply(*dt, t));
				e1->v = vector_subtract(e1->v, vector_scalar_multiply(normal, vector_dot_product(e1->v, normal)));
				*dt = vector_subtract(*dt, vector_scalar_multiply(normal, vector_dot_product(*dt, normal)));
			}
		}
	}
}

Minkowski_Box calculate_minkowski_sum(Entity e1, Entity e2)
{
	Minkowski_Box mink = {};
	Minkowski_Box box1 = {e1.p, e1.hitbox.x > 0 ? e1.hitbox.x : e1.w, e1.hitbox.y > 0 ? e1.hitbox.y : e1.h};
	Minkowski_Box box2 = {e2.p, e2.hitbox.x > 0 ? e2.hitbox.x : e2.w, e2.hitbox.y > 0 ? e2.hitbox.y : e2.h};
	mink.p.x = (box1.p.x - (box1.w * 0.5f)) - (box2.p.x + (box2.w * 0.5f));
	mink.p.y = (box1.p.y - (box1.h * 0.5f)) - (box2.p.y + (box2.h * 0.5f));
	mink.w = box1.w + box2.w;
	mink.h = box1.h + box2.h;

	return mink;
}

int is_collision(Minkowski_Box mink)
{
	return (
		mink.p.x <= 0 && mink.p.x + mink.w >= 0 &&
		mink.p.y <= 0 && mink.p.y + mink.h >= 0
	);
}

void detect_collisions(Entity *entity)
{
	int old_n_collisions = entity->n_collisions;
	int old_collision_ids[old_n_collisions];

	for (int i = 0; i < old_n_collisions; ++i) {
		old_collision_ids[i] = entity->collision[i].id;
	}

	V2 normal = {};
	entity->n_collisions = 0;

	for (Entity *other_entity = entities.buffer; other_entity != entities.p; ++other_entity) {
		if (entity->id != other_entity->id) {
			Minkowski_Box mink = calculate_minkowski_sum(*entity, *other_entity);

			if (is_collision(mink)) {
				normal = find_normal(mink);

				Collision collision;
				collision.id = other_entity->id;
				collision.type = other_entity->type;
				collision.normal = normal;
				collision.mink = mink;
				collision.new = true;

				for (int i = 0; i < old_n_collisions; ++i) {
					if (collision.id == old_collision_ids[i]) {
						collision.new = false;
					}
				}

				int n = entity->n_collisions++;
				entity->collision[n] = collision;

				if (normal.y == -1.0f) {
					entity->on_ground = true;
				}
			}
		}
	}
}

V2 calculate_velocity(Entity *e)
{
	V2 a = e->a;
	V2 v = e->v;

	a = vector_normalize(a);
	a.x *= e->speed.x;
	a.y *= e->speed.y;
	a = vector_scalar_multiply(a, frame_dt);

	v = vector_add(v, a);
	v.x *= pow(e->damp.x, frame_dt);
	v.y *= pow(e->damp.y, frame_dt);

	return v;
}

V2 calculate_position(V2 a, V2 v)
{
	a = vector_scalar_multiply(a, 0.5f * square(frame_dt));
	v = vector_scalar_multiply(v, frame_dt);
	V2 dt_p = vector_add(v, a);

	return dt_p;
}

void apply_jump(Entity *e, float force)
{
	if (e->on_ground) {
		e->v.y -= force;
		e->on_ground = false;
	}
}

void apply_gravity(Entity *entity)
{
	if (!entity->on_ladder) {
		entity->a.y = 1.0f;
	}
}

Collision *search_collisions(Entity *e, enum EntityType type)
{
	for (int i = 0; i < e->n_collisions; ++i) {
		if (e->collision[i].type == type) {
			return &e->collision[i];
		}
	}

	return NULL;
}

void transition_to_ladder(Entity *e)
{
	Collision *ladder_collision = search_collisions(e, ladder);
	Collision *platform_collision = search_collisions(e, platform);
	Minkowski_Box m = ladder_collision->mink;

	if (!e->on_ladder) {
		if ((e->motion_input.down && ((int)(m.p.y + m.h) < m.h - e->h)) ||
			(e->motion_input.up && ((int)(m.p.y + m.h) > e->h))) {
			if (platform_collision && !e->platform_transition) {
				if (m.p.x + m.w > (m.w * 0.5f) - 4 && m.p.x + m.w < (m.w * 0.5f) + 4) {
					e->a.y = 0.0f;
					e->v.x = 0.0f;
					e->on_ladder = true;
					e->ladder_transition = true;
				}
			}
		}
	} else {
		if (e->ladder_transition) {
			if (m.p.x + m.w < (m.w * 0.5) - 0.1f) {
				e->a.x = 1.0f;
			} else if (m.p.x + m.w > (m.w * 0.5) + 0.1f) {
				e->a.x = -1.0f;
			} else {
				e->ladder_transition = false;
				e->v.x = 0.0f;
			}
		}
	}
}

void transition_to_platform(Entity *e)
{
	Collision *platform_collision = search_collisions(e, platform);
	Entity advance_entity = *e;
	advance_entity.p.y += 5;
	detect_collisions(&advance_entity);
	Collision *advance_platform_collision = search_collisions(&advance_entity, platform);

	if (!platform_collision) {
		if (advance_platform_collision) {
			if ((e->motion_input.left || e->motion_input.right) && !e->motion_input.up) {
				e->platform_transition = true;
				e->v.y = -50.0f;
			}
		}

		if (e->platform_transition) {
			off_ladder(e);
		}
	} else {
		Minkowski_Box mp = platform_collision->mink;

		if (e->motion_input.left || e->motion_input.right) {
			if (!e->ladder_transition && !e->platform_transition) {
				int to_platform_top = (int)mp.p.y + mp.h;

				if (to_platform_top < 5) {
					e->platform_transition = true;
					e->a = get_accel(e->motion_input);
				}
			}
		}

		if (e->platform_transition) {
			e->a = get_accel(e->motion_input);
			e->a.y = -1.0f;
		}
	}
}

void off_ladder(Entity *e)
{
	e->on_ladder = false;
	e->platform_transition = false;
	e->damp.x = 0.0001f;
	e->damp.y = 0.9f;
}

void climb_ladder(Entity *e)
{
	Collision *ladder_collision = search_collisions(e, ladder);

	if (!ladder_collision) {
		off_ladder(e);
	} else {
		Minkowski_Box m = ladder_collision->mink;
		transition_to_ladder(e);

		if (e->on_ladder) {
			if (!e->ladder_transition) {
				e->a.x = 0.0f;
			}

			e->damp.x = 0.0001f;
			e->damp.y = 0.0001f;

			int to_ladder_bottom = (int)m.p.y + m.h;
			int ladder_bottom = m.h - e->h;

			transition_to_platform(e);

			if (to_ladder_bottom > ladder_bottom) {
				off_ladder(e);
			}
		}
	}
}

void reset_animation_cycle(Entity *e)
{
	e->clock = 0.0f;
}

void offset_timer(Entity *e, int *offset)
{
	float cycle = 0.5f;
	float v;

	if (e->anim_state == climbing) {
		v = fabs(e->v.y);
	} else if (e->anim_state == dead || e->anim_state == winning) {
		v = 60.0f;
	} else {
		v = fabs(e->v.x);
	}

	if (e->clock < (cycle * 0.5f) / (v * frame_dt)) {
		*offset = 1;
	} else if (e->clock > cycle / (v * frame_dt)) {
		reset_animation_cycle(e);
	}
}

Bitmap *get_animation_frame(Entity *e)
{
	int index = 0;
	int offset = 0;

	switch (e->anim_state) {
	case none:
		break;

	case standing:
		index = 0;
		reset_animation_cycle(e);
		break;

	case walking:
		index = 0;
		offset_timer(e, &offset);
		break;

	case jumping:
	case landing:
		index = 0;
		break;

	case climbing:
		index = 2;
		offset_timer(e, &offset);
		break;
	case dead:
		index = 6;
		offset_timer(e, &offset);
		break;
	case winning:
		index = 10;
		offset_timer(e, &offset);
		break;
	}

#if 1 // missing climbing and dead frames for NPCs
	if ((e->anim_state == climbing || e->anim_state == dead) && (e->type == 1 || e->type == 2)) {
		offset = 0;
		index = 0;
	}
#endif

	char **bitmap_list = entity_bitmap_list[e->type];
	Bitmap *bitmap = (Bitmap *)hash_lookup(bitmap_table, bitmap_list[index + offset]);

	return bitmap;
}

void update_animation_cycle(Entity *e)
{
	e->clock += frame_dt;

	if (e->clock > 5.0f) {
		e->clock = 0.0f;
	}
}

void load_entities(enum EntityType et)
{
	FILE *fp = fopen("entities.dat", "r");
	int n_levels;
	fread(&n_levels, 4, 1, fp);
	int n_values;
	fread(&n_values, 4, 1, fp);
	int n_entities;
	fread(&n_entities, 4, 1, fp);

	for (int i = 0 ; i < n_entities; ++i) {
		int n;
		Entity e = {};
		int id, x, y, w, h;
		enum EntityType type;
		fread(&id, 4, 1, fp);
		fread(&type, 4, 1, fp);
		if (et == all || et == type) {
			Bitmap *bitmap;
			bitmap = (Bitmap *)hash_lookup(bitmap_table, entity_bitmap_list[type][0]);

			if (bitmap) {
				w = bitmap->w;
				h =bitmap->h;
			}

			fread(&n, 4, 1, fp);
			x = (float)n;

			fread(&n, 4, 1, fp);
			y = (float)n;

			fread(&n, 4, 1, fp);
			if (n > 0)
				w = n;

			fread(&n, 4, 1, fp);
			if (n > 0)
				h = n;

			if (type == player) {
				e = player_defaults;
			} else if (type == hotdog || type == egg) {
				e = npc_defaults;
			} else if (type == top_bun || type == tomato || type == meat || type == bottom_bun) {
				e = burger_defaults;
			} else if (type == plate) {
				e = plate_defaults;
			}

			if (type == ladder || type == door) {
				e.permeable = true;
			}

			e.id = id;
			e.type = type;
			set_position(&e, x, y);
			set_dimensions(&e, w, h);
			push_memory(&entities, &e);
		}
	}
}

void add_collision(
	Entity* subject_entity,
	Entity* object_entity,
	V2 normal,
	Minkowski_Box mink)
{
	Collision collision;
	collision.id = object_entity->id;
	collision.type = object_entity->type;
	collision.normal = normal;
	collision.mink = mink;
	int n = subject_entity->n_collisions++;
	subject_entity->collision[n] = collision;
}

void reap_entities()
{
	int to_reap_ids[MAX_ENTITIES];
	int n_to_reap = 0;

	for (Entity *e = entities.buffer; e != entities.p; ++e) {
		if (e->dead) {
			to_reap_ids[n_to_reap++] = e->id;
		}
	}

	for (int i = 0; i < n_to_reap; ++i) {
		Entity *e = get_entity(to_reap_ids[i]);
		if (e->p.y > 400) {
			delete_memory(&entities, e);
		}
	}
}

void kill_entities()
{
	for (Entity *e = entities.buffer; e != entities.p; ++e) {
		if (e->type == hotdog || e->type == egg) {
			for (int i = 0; i < e->n_collisions; ++i) {
				Collision c = e->collision[i];

				if (c.normal.y == 1.0f &&
					(c.type == top_bun ||
					 c.type == tomato ||
					 c.type == meat ||
					 c.type == bottom_bun)) {
					 e->dead = true;
				}
			}
		}

		if (e->type == player) {
			for (int i = 0; i < e->n_collisions; ++i) {
				Collision c = e->collision[i];

				if (c.type == hotdog || c.type == egg) {
					e->dead = true;
				}
			}
		}
	}
}

void spawn_npc()
{
	static float timer = 0.0f;
	Entity *door_list[4];
	int n_doors = 0;
	int n_npcs = 0;

	for (Entity *e = entities.buffer; e != entities.p; ++e) {
		if (e->type == egg || e->type == hotdog) {
			++n_npcs;
		} else if (e->type == door && n_doors < array_size(door_list)) {
			door_list[n_doors++] = e;
		}
	}

	int r = rand() % n_doors;
	Entity *door = door_list[r];

	if (n_npcs < 4) {
		timer += frame_dt;

		if (timer > 5) {
			if (n_npcs % 2) {
				add_entity(hotdog, door->p.x, door->p.y);
			} else {
				add_entity(egg, door->p.x, door->p.y);
			}

			timer = 0.0f;
		}
	}
}

void update_entities()
{
	win = true;

	for (Entity *e = entities.buffer; e != (Entity *)entities.p; ++e) {
		update_animation_cycle(e);

		if (e->type == top_bun || e->type == tomato || e->type == meat || e->type == bottom_bun) {
			move_burger_component(e);
			if (search_collisions(e, platform) || e->n_collisions == 0)
				win = false;
		}

		if (e->movable) {
			move_entity(e);
		}
	}

	spawn_npc();
	kill_entities();
	reap_entities();
}

Entity *add_entity(enum EntityType type, float x, float y)
{
	Entity e = {};
	Bitmap *bitmap = (Bitmap *)hash_lookup(bitmap_table, entity_bitmap_list[type][0]);

	if (type == egg || type == hotdog) {
		e = npc_defaults;
	}

	int id = 0;
	for (Entity *e = entities.buffer; e != entities.p; ++e) {
		if (e->id > id)
			id = e->id;
	}

	e.id = ++id;
	e.type = type;
	set_position(&e, x, y);
	set_dimensions(&e, bitmap->w, bitmap->h);
	Entity *p = (Entity *)push_memory(&entities, &e);
	return p;
}

Entity *get_entity(int id)
{
	Entity *p = entities.buffer;

	while (p != entities.p) {
		if (p->id == id) {
			return p;
		}

		++p;
	}

	return NULL;
}

void draw_entity(Entity *e)
{
	Bitmap *p = get_animation_frame(e);

	if (p) {
		Bitmap bitmap = *p;

		if (e->direction == left) {
			bitmap = flip_bitmap(bitmap);
		}

		for (int h = 0; h < e->h; h += bitmap.h) {
			for (int w = 0; w < e->w; w += bitmap.w) {
				draw_bitmap(
					bitmap,
					display_bitmap,
					(int)roundf(e->p.x - (e->w * 0.5f)) + w,
					(int)roundf(e->p.y - (e->h * 0.5f)) + h,
					0, 0, 0, 0,
					-1
				);
			}
		}
	}
}

void draw_screen()
{
	for (Entity *e = entities.buffer; e != entities.p; ++e) {
		if (e->type != hotdog && e->type != egg && e->type != player) {
			draw_entity(e);
		}
	}

	for (Entity *e = entities.buffer; e != entities.p; ++e) {
		if (e->type == hotdog || e->type == egg) {
			draw_entity(e);
		}
	}

	Entity *player = get_entity(0);

	if (player) {
		draw_entity(player);
	}
}

void move_burger_component(Entity *e)
{
	Collision *platform_collision = search_collisions(e, platform);

	if (!platform_collision) {
		e->movable = true;
	} else {
		if (e->dest.y == 0.0f) {
			detect_collisions(e);
			Collision *player_collision = search_collisions(e, player);
			Collision *top_bun_collision = search_collisions(e, top_bun);
			Collision *tomato_collision = search_collisions(e, tomato);
			Collision *meat_collision = search_collisions(e, meat);
			Collision *bottom_bun_collision = search_collisions(e, bottom_bun);

			if (player_collision && player_collision->new && player_collision->normal.y == 1.0f) {
				e->dest.y = e->p.y + 3.0f;
			} else if ((top_bun_collision && top_bun_collision->normal.y == 1.0f) ||
					   (tomato_collision && tomato_collision->normal.y == 1.0f) ||
					   (meat_collision && meat_collision->normal.y == 1.0f) ||
					   (bottom_bun_collision && bottom_bun_collision->normal.y == 1.0f)) {
				e->dest.y = e->p.y + 10.0f;
			}
		} else {
			float diff = e->dest.y - e->p.y;

			if (fabs(diff) > 1.0f) {
				e->a.y = (diff >= 0) ? 1.0f : -1.0f;
				e->v = calculate_velocity(e);
				V2 dt_p = calculate_position(e->a, e->v);
				e->p = vector_add(e->p, dt_p);
				e->movable = false;
			} else {
				e->dest.y = 0.0f;
				e->v.y = 0.0f;
			}
		}

		if (platform_collision->mink.p.y + (float)get_entity(platform_collision->id)->h > -0.5f) {
			e->a.y = 1.0f;
			e->v = calculate_velocity(e);
			V2 dt_p = calculate_position(e->a, e->v);
			e->p = vector_add(e->p, dt_p);
		}
	}
}

void move_entity(Entity *entity)
{
	Entity new_entity = *entity;

	if (!new_entity.dead && new_entity.anim_state != winning) {
		if (new_entity.type == player) {
			new_entity.anim_state = standing;
			new_entity.motion_input = get_player_motion_input();
		} else if (new_entity.type == egg || new_entity.type == hotdog) {
			new_entity.motion_input = get_npc_motion_input(&new_entity);
		}
	} else {
		new_entity.motion_input = (MotionInput){0};
	}

	if (!(new_entity.type == player && new_entity.dead)) {
		new_entity.a = get_accel(new_entity.motion_input);
		apply_gravity(&new_entity);
	}

	if (new_entity.a.x < 0) new_entity.direction = left;
	else if (new_entity.a.x > 0) new_entity.direction = right;

	if (new_entity.motion_input.down || new_entity.motion_input.up || new_entity.on_ladder) {
		climb_ladder(&new_entity);
	}

	if (new_entity.motion_input.jump) {
		apply_jump(&new_entity, 200.0f);
		new_entity.on_ground = false;
		new_entity.anim_state = jumping;
	}

	if (new_entity.on_ladder) {
		new_entity.on_ground = false;
		new_entity.anim_state = climbing;
	}

	new_entity.v = calculate_velocity(&new_entity);
	V2 dt_p = calculate_position(new_entity.a, new_entity.v);

	if (fabs(dt_p.x) > 0.1f && new_entity.on_ground) {
		new_entity.anim_state = walking;
	}

	if (new_entity.dead) {
		new_entity.anim_state = dead;
		if (new_entity.type == player) {
			new_entity.v = (V2){0};
			new_entity.a = (V2){0};
		}
	}

	new_entity.p = vector_add(new_entity.p, dt_p);
	detect_collisions(&new_entity);

	if (!new_entity.on_ladder && !new_entity.dead) {
		resolve_collision(entity, &new_entity, &dt_p);
	}

	*entity = new_entity;
}

void set_position(Entity *e, float x, float y)
{
	e->p.x = x;
	e->p.y = y;
}

void set_dimensions(Entity *e, int w, int h)
{
	if (w > 0)
		e->w = w;

	if (h > 0)
		e->h = h;
}

void reset_npcs()
{
	int to_reap_ids[MAX_ENTITIES];
	int n_to_reap = 0;

	for (Entity *e = entities.buffer; e != entities.p; ++e) {
		if (e->type == egg || e->type == hotdog) {
			to_reap_ids[n_to_reap++] = e->id;
		}
	}

	for (int i = 0; i < n_to_reap; ++i) {
		Entity *e = get_entity(to_reap_ids[i]);
		delete_memory(&entities, e);
	}
}

void reset_screen()
{
	static float timer = 0.0;

	timer += frame_dt;

	if (timer > 3) {
		reset_npcs();
		reset_npcs_state = true;
		playing = false;
		get_entity(0)->dead = false;
		clear_bitmap(display_bitmap, 0);
		draw_string(display_bitmap, 0, 0, "READY", 1.0f, 0xffffff, 1);

		if (timer > 6) {
			timer = 0.0f;
			playing = true;
			reset_npcs_state = false;
		}
	}
}

void start_screen()
{
	clear_bitmap(display_bitmap, 0);
	draw_string(display_bitmap, 0, 0, "BURGER GIRL", 1.0f, 0xffffff, 1);
	draw_string(display_bitmap, 0, 15, "<ENTER> TO PLAY", 1.0f, 0xffffff, 1);
}

void reset_game()
{
	playing = false;
	win = false;
	start_screen_state = true;
	reset_npcs_state = false;
	entities.p = entities.buffer;
	entities.n_elems = 0;
	load_entities(all);
}

void win_screen()
{
	static float timer = 0.0;

	timer += frame_dt;

	draw_string(display_bitmap, 0, 0, "YOU WIN!", 1.0f, 0xffffff, 1);

	if (timer > 5) {
		reset_game();
		timer = 0.0f;
	}

}

void main_loop()
{
	struct timespec current_time;
	uint32_t accumulated_time = 0;
	uint32_t target_time = frame_dt * 1000000000; /* nanoseconds */

	clock_gettime(CLOCK_REALTIME, &current_time);

	while (running) {
		if (accumulated_time > target_time) {
			old_input = new_input;
			get_input();
			process_ui_input();

			if (get_entity(0)->dead || reset_npcs_state) {
				reset_screen();
			}

			if (start_screen_state) {
				start_screen();
				if (new_input.key_return) {
					start_screen_state = false;
					playing = true;
				}
			}

			if (playing) {
				update_entities();
				clear_bitmap(display_bitmap, 0);
				background_bitmap = *(Bitmap *)hash_lookup(bitmap_table, "background");
				draw_bitmap(background_bitmap, display_bitmap, 0, 0, 0, 0, 0, 0, -1);
				draw_screen();
			}

			if (win) {
				Entity *player = get_entity(0);
				player->anim_state = winning;
				reset_npcs();
				win_screen();
			}

			blit_display();

#if 0
			printf("%f\n", accumulated_time / 1000000000.0f);
#endif
			accumulated_time -= target_time;
			usleep(4000);
		}

		uint32_t last_time = current_time.tv_nsec;
		clock_gettime(CLOCK_REALTIME, &current_time);
		int32_t diff_time = current_time.tv_nsec - last_time;

		if (diff_time < 0) {
			diff_time += 1000000000;
		}

		accumulated_time += diff_time;
	}
}

int main()
{
	srand(time(NULL));
	init_display();
	init_memory(1024 * 1024);
	load_win_bmps();
	chars_bitmap = read_win_bmp("assets/chars/chars.bmp");
	bitmap_buffer = reserve_memory(1024 * 100, 1);
	entities = reserve_memory(MAX_ENTITIES, sizeof(Entity));
	load_entities(all);
	display_bitmap.w = DISPLAY_WIDTH;
	display_bitmap.h = DISPLAY_HEIGHT;
	display_bitmap.data = (unsigned char *)malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * 4);
	display.data = display_bitmap.data;
	main_loop();
	free(memory.buffer);

	return 0;
}


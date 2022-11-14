#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include <curses.h>
#include <string.h>
#include <time.h>

#ifndef COLOR_BLACK
#define start_color()
#define COLOR_PAIR(a) 0
#endif

#define PART_COUNT 10
#define PART_VARIANCE 5
#define SIN_TABLE_SIZE 350
#define COLOR_MIXED -1
#define TABLE_SCALE(val) ((int)(val / M_PI * SIN_TABLE_SIZE))
#define BOUNDCHAR(win, y, x, char)                              \
        (((y) >= 0 && (y) < LINES && (x) >= 0 && (x) < COLS) ?  \
         mvwaddch((win), (y), (x), (char)) : 0)
#define PAT_RAND_SEQ    0x01

typedef struct particle_t
{
        double y_scale;
        double x_scale;
        double x_inc;
        double x;
        int old_y, old_x;
        int direction;
        int color;
        int sequence;
        int pattern_inc;
} particle_t;

typedef struct pattern_t
{
        char *string;
        int mask;
} pattern_t;

typedef struct object_t
{
        int rel_y, rel_x;
        int part_count;
        pattern_t *pattern;
        int pat_cycle;
        int color;
        int done_count;
        int life;
        double y_mag, x_mag;
        particle_t *particles;
} object_t;

typedef struct object_list_t
{
        object_t *object;
        struct object_list_t *next;
} object_list_t;

static pattern_t Patterns[] = {
        {"......+++++++******@@@@@@@", 0},
        {"|\\-/", PAT_RAND_SEQ},
        {".......ooooOOOOoooo", 0},
        {"()", 0},
        {"-=", PAT_RAND_SEQ},
        {"*", 0}
};

#define PATTERN_SIZE (sizeof(Patterns) / sizeof(Patterns[0]))

static int ColorId = 1;
#define RAND_COLOR() (lrand48() % ColorId + 1)

static void
FreeMemory(object_list_t * item)
{
        free((char *)item->object->particles);
        free((char *)item->object);
        free((char *)item);
}

static void
DeleteObject(object_list_t * zap, object_list_t ** top)
{
        object_list_t *new, *last;

        if (zap == *top)
        {
                new = zap->next;
                FreeMemory(zap);
                *top = new;
                return;
        }
        new = *top;
        while (new != zap)
        {
                last = new;
                new = new->next;
        }
        last->next = zap->next;
        FreeMemory(zap);
}

static void
DisplayObject(WINDOW * win, double *sin_table, object_t * object)
{
        int i;
        int draw_y, draw_x;
        particle_t *p;
        double y;
        int sym;

        if (--object->life < 0)
        {
                for (i = 0; i < object->part_count; ++i)
                {
                        p = &object->particles[i];
                        BOUNDCHAR(win, p->old_y, p->old_x, ' ');
                }
                return;
        }
        for (i = 0; i < object->part_count; ++i)
        {
                p = &object->particles[i];
                if (p->x < M_PI)
                {
                        BOUNDCHAR(win, p->old_y, p->old_x, ' ');
                        y = sin_table[TABLE_SCALE(p->x)] * p->y_scale;
                        draw_y = object->rel_y - y;
                        draw_x = object->rel_x + p->direction * p->x * p->x_scale;
                        p->sequence += p->pattern_inc;
                        if (p->sequence < 0)
                                p->sequence = object->pat_cycle - 1;
                        else if (p->sequence >= object->pat_cycle)
                                p->sequence = 0;
                        sym = object->pattern->string[p->sequence];
                        BOUNDCHAR(win, draw_y, draw_x, sym | COLOR_PAIR(p->color));
                        p->old_y = draw_y;
                        p->old_x = draw_x;
                        p->x += p->x_inc;
                        if (p->x >= M_PI)
                        {
                                BOUNDCHAR(win, draw_y, draw_x, ' ');
                                ++object->done_count;
                        }
                }
        }
}

static void
DoObjectList(WINDOW * win, double *sin_table, object_list_t ** top)
{
        object_list_t *now, *old;

        now = *top;
        while (now)
        {
                DisplayObject(win, sin_table, now->object);
                old = now;
                now = now->next;
                if (old->object->done_count >= old->object->part_count || old->object->life < 0)
                        DeleteObject(old, top);
        }
        usleep(1000 * 100);
}

static void
BuildParticles(object_t * object)
{
        int i;
        particle_t *p;

        for (i = 0; i < object->part_count; ++i)
        {
                p = &object->particles[i];
                p->y_scale = drand48() * (LINES - 1) * object->y_mag;
                p->x_scale = drand48() * COLS / 6.0 * object->x_mag;
                p->x_inc = drand48() * 0.05 + 0.1;
                p->x = 0.0;
                p->old_y = 0;
                p->old_x = 0;
                p->direction = lrand48() & 1 ? -1 : 1;
                if (object->pattern->mask & PAT_RAND_SEQ)
                {
                        p->sequence = lrand48() % object->pat_cycle;
                        p->pattern_inc = -p->direction;
                }
                else
                {
                        p->sequence = 0;
                        p->pattern_inc = 1;
                }
                if (object->color == COLOR_MIXED)
                        p->color = RAND_COLOR();
                else
                        p->color = object->color;
        }
}

static object_t *
BuildObject(void)
{
        object_t *object;

        object = (object_t *) malloc(sizeof(object_t));
        object->part_count = lrand48() % PART_VARIANCE + PART_COUNT;
        object->particles = (particle_t *) malloc(object->part_count * sizeof(particle_t));
        object->pattern = &Patterns[lrand48() % PATTERN_SIZE];
        object->pat_cycle = strlen(object->pattern->string);
        object->rel_y = LINES - lrand48() % 15;
        object->rel_x = lrand48() % (COLS / 4 * 3) + COLS / 4 * 1 / 2;
        object->color = lrand48() % 4 ? RAND_COLOR() : COLOR_MIXED;
        object->done_count = 0;
        object->y_mag = drand48() * 0.9 + 0.5;
        object->x_mag = drand48() * 0.9 + 0.5;
        object->life = lrand48() % 15 + 10;
        BuildParticles(object);

        return object;
}

static void
AddObject(object_list_t ** top)
{
        object_list_t *new_top;

        new_top = (object_list_t *) malloc(sizeof(object_list_t));
        new_top->object = BuildObject();
        new_top->next = *top;
        *top = new_top;
}

static void
DoSinTable(double *table)
{
        int i;
        double inc;
        double current;

        inc = M_PI / SIN_TABLE_SIZE;
        for (i = 0, current = 0.0; i < SIN_TABLE_SIZE; ++i, current += inc)
        {
                table[i] = sin(current);
                if (!(i % 66))
                {
                        putchar('.');
                        fflush(stdout);
                }
        }
}

static void
InitColors(void)
{
#ifdef COLOR_BLACK
        init_pair(ColorId++, COLOR_RED, COLOR_BLACK);
        init_pair(ColorId++, COLOR_GREEN, COLOR_BLACK);
        init_pair(ColorId++, COLOR_YELLOW, COLOR_BLACK);
        init_pair(ColorId++, COLOR_BLUE, COLOR_BLACK);
        init_pair(ColorId++, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(ColorId++, COLOR_CYAN, COLOR_BLACK);
        init_pair(ColorId, COLOR_WHITE, COLOR_BLACK);
#endif
}

int
main(int argc, char *argv[])
{
        double sin_table[SIN_TABLE_SIZE];
        object_list_t *top;
        time_t show_over;

        printf("wait.");
        fflush(stdout);
        DoSinTable(sin_table);
        srand48((long)time((time_t *) 0));
        initscr();
        start_color();
        InitColors();
        top = (object_list_t *) malloc(sizeof(object_list_t));
        top->object = BuildObject();
        top->next = 0;
        show_over = time((time_t *) 0) + 10 * 60;
        while (time((time_t *) 0) < show_over)
        {
                if (!(lrand48() % 20))
                        AddObject(&top);
                else if (!top)
                {
                        sleep(1);
                        AddObject(&top);
                }
                DoObjectList(stdscr, sin_table, &top);
                wrefresh(stdscr);
        }
        endwin();
        return 0;
}

#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_WORDS   8
#define MAX_LEN     32
#define MAX_LET     10
#define MAX_TEXT    512

typedef struct {
    int nwords;
    int nadd;
    int nlet;
    int result;
    int len[MAX_WORDS];
    int word[MAX_WORDS][MAX_LEN];
} Puzzle;

static int parse_puzzle(const char *s, Puzzle *p)
{
    int id[26];
    int i, c;
    int nwords = 0, len = 0;

    for (i = 0; i < 26; i++)
        id[i] = -1;

    p->nlet = 0;
    p->nwords = 0;

    for (i = 0; ; i++) {
        c = (unsigned char)s[i];

        if (c == '+' || c == '=' || c == '\0') {
            if (len > 0) {
                if (nwords >= MAX_WORDS)
                    return 0;
                p->len[nwords] = len;
                nwords++;
                len = 0;
            }
            if (c == '\0')
                break;
        } else if (c >= 'A' && c <= 'Z') {
            int k = c - 'A';

            if (id[k] < 0) {
                if (p->nlet >= MAX_LET)
                    return 0;
                id[k] = p->nlet++;
            }
            if (len >= MAX_LEN - 1)
                return 0;
            p->word[nwords][len++] = id[k];
        } else if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {

        } else {
            return 0;
        }
    }

    if (nwords < 2)
        return 0;

    p->nwords = nwords;
    p->nadd = nwords - 1;
    p->result = nwords - 1;

    {
        int maxadd = 0;
        for (i = 0; i < p->nadd; i++)
            if (p->len[i] > maxadd)
                maxadd = p->len[i];
        if (p->len[p->result] < maxadd)
            return 0;
    }

    return 1;
}

static int valid_all(const Puzzle *p, const int *dig)
{
    long long sum = 0, val;
    int w, pos;

    for (w = 0; w < p->nadd; w++) {
        if (p->len[w] > 1 && dig[p->word[w][0]] == 0)
            return 0;
        val = 0;
        for (pos = 0; pos < p->len[w]; pos++)
            val = val * 10 + dig[p->word[w][pos]];
        sum += val;
    }

    w = p->result;
    if (p->len[w] > 1 && dig[p->word[w][0]] == 0)
        return 0;
    val = 0;
    for (pos = 0; pos < p->len[w]; pos++)
        val = val * 10 + dig[p->word[w][pos]];

    return val == sum;
}

static void build_answer(const Puzzle *p, const int *dig, char *out)
{
    int n = 0, w, pos;

    for (w = 0; w < p->nwords; w++) {
        if (w > 0) {
            if (w == p->result) {
                out[n++] = ' '; out[n++] = '='; out[n++] = ' ';
            } else {
                out[n++] = ' '; out[n++] = '+'; out[n++] = ' ';
            }
        }
        for (pos = 0; pos < p->len[w]; pos++)
            out[n++] = (char)('0' + dig[p->word[w][pos]]);
    }
    out[n] = '\0';
}

static const Puzzle *g_p;
static int g_digit[MAX_LET];
static int g_solution[MAX_LET];
static int g_used[10];
static int g_found;

static int g_reslen;

static int g_add_col[MAX_LEN][MAX_LET];
static int g_add_cnt[MAX_LEN];
static int g_res_col[MAX_LEN];
static int g_can_zero[MAX_LET];
static int g_used_count;

static int leading_ok(void)
{
    int w;
    for (w = 0; w < g_p->nwords; w++)
        if (g_p->len[w] > 1 && g_digit[g_p->word[w][0]] == 0)
            return 0;
    return 1;
}

static void v3_column(int c, int carry);

static void v3_assign(int c, int carry, int k, const int *letters, int count)
{
    int d, l;

    if (g_found)
        return;

    if (k == count) {
        long long sum = carry;
        int w, r, rd;

        for (w = 0; w < g_p->nadd; w++)
            if (c < g_p->len[w])
                sum += g_digit[g_p->word[w][g_p->len[w] - 1 - c]];

        r = g_res_col[c];
        rd = (int)(sum % 10);

        if (g_digit[r] >= 0) {

            if (g_digit[r] == rd)
                v3_column(c + 1, (int)(sum / 10));
        } else if (!g_used[rd] && (rd != 0 || g_can_zero[r])) {

            g_digit[r] = rd;
            g_used[rd] = 1;
            g_used_count++;
            v3_column(c + 1, (int)(sum / 10));
            g_used_count--;
            g_used[rd] = 0;
            g_digit[r] = -1;
        }
        return;
    }

    l = letters[k];

    if (g_used_count == 9) {
        for (d = 0; d <= 9; d++)
            if (!g_used[d])
                break;
        if (d == 0 && !g_can_zero[l])
            return;
        g_used[d] = 1;
        g_used_count++;
        g_digit[l] = d;
        v3_assign(c, carry, k + 1, letters, count);
        g_digit[l] = -1;
        g_used_count--;
        g_used[d] = 0;
        return;
    }

    for (d = 0; d <= 9; d++) {
        if (g_used[d])
            continue;
        if (d == 0 && !g_can_zero[l])
            continue;
        g_used[d] = 1;
        g_used_count++;
        g_digit[l] = d;
        v3_assign(c, carry, k + 1, letters, count);
        g_digit[l] = -1;
        g_used_count--;
        g_used[d] = 0;
        if (g_found)
            return;
    }
}

static void v3_column(int c, int carry)
{
    int i;
    int letters[MAX_LET];
    int count = 0;

    if (g_found)
        return;

    if (c == g_reslen) {
        if (carry == 0 && leading_ok()) {
            for (i = 0; i < g_p->nlet; i++)
                g_solution[i] = g_digit[i];
            g_found = 1;
        }
        return;
    }

    for (i = 0; i < g_add_cnt[c]; i++) {
        int l = g_add_col[c][i];
        if (g_digit[l] < 0)
            letters[count++] = l;
    }

    v3_assign(c, carry, 0, letters, count);
}

static char g_answer[MAX_TEXT];

static void prepare_columns(const Puzzle *p)
{
    int c, w, i, j;

    for (c = 0; c < MAX_LEN; c++) {
        g_add_cnt[c] = 0;
        g_res_col[c] = -1;
    }
    for (i = 0; i < MAX_LET; i++)
        g_can_zero[i] = 1;

    for (w = 0; w < p->nwords; w++)
        if (p->len[w] > 1)
            g_can_zero[p->word[w][0]] = 0;

    for (w = 0; w < p->nadd; w++) {
        for (c = 0; c < p->len[w]; c++) {
            int l = p->word[w][p->len[w] - 1 - c];
            int cnt = g_add_cnt[c];

            for (j = 0; j < cnt; j++)
                if (g_add_col[c][j] == l)
                    break;
            if (j == cnt)
                g_add_col[c][cnt++] = l;
            g_add_cnt[c] = cnt;
        }
    }

    for (c = 0; c < p->len[p->result]; c++)
        g_res_col[c] = p->word[p->result][p->len[p->result] - 1 - c];
}

const char *solve_rebus(const char *puzzle)
{
    Puzzle p;
    int i;

    g_found = 0;

    if (!parse_puzzle(puzzle, &p))
        return NULL;

    for (i = 0; i < MAX_LET; i++)
        g_digit[i] = -1;
    for (i = 0; i < 10; i++)
        g_used[i] = 0;
    g_used_count = 0;

    g_p = &p;
    g_reslen = p.len[p.result];
    prepare_columns(&p);
    v3_column(0, 0);

    if (!g_found)
        return NULL;
    if (!valid_all(&p, g_solution))
        return NULL;

    build_answer(&p, g_solution, g_answer);
    return g_answer;
}

static int check_answer_string(const char *s)
{
    long long sum = 0, num = 0;
    int len = 0, first = 0, seen_eq = 0, seen_digit = 0;
    const char *p;

    for (p = s; *p; p++) {
        char c = *p;

        if (c >= '0' && c <= '9') {
            if (len == 0)
                first = c;
            len++;
            num = num * 10 + (c - '0');
            seen_digit = 1;
        } else if (c == '+') {
            if (len > 1 && first == '0')
                return 0;
            sum += num;
            num = 0;
            len = 0;
        } else if (c == '=') {
            if (len > 1 && first == '0')
                return 0;
            sum += num;
            num = 0;
            len = 0;
            seen_eq = 1;
        } else if (c == ' ') {

        } else {
            return 0;
        }
    }

    if (!seen_digit || !seen_eq)
        return 0;
    if (len > 1 && first == '0')
        return 0;

    return sum == num;
}

static const char *TESTS[] = {
    "SEND + MORE = MONEY",
    "BE + BE = MOO",
    "BIG + CAT = LION",
    "FORTY + TEN + TEN = SIXTY",
    "DONALD + GERALD = ROBERT",
    "ELEVEN + NINE + FIVE + FIVE = THIRTY",
    "SATURN + URANUS + NEPTUNE + PLUTO = PLANETS"
};

#define NTESTS ((int)(sizeof(TESTS) / sizeof(TESTS[0])))

static double now_ms(void)
{
    return (double)clock() * 1000.0 / (double)CLOCKS_PER_SEC;
}

static void run_benchmark(void)
{
    int t, i;
    double total = 0.0;

    printf("V3: + запрет ведущих нулей, готовые столбцы, последняя цифра\n\n");
    printf("Решения:\n");
    for (t = 0; t < NTESTS; t++) {
        const char *ans = solve_rebus(TESTS[t]);

        if (ans == NULL || !check_answer_string(ans)) {
            printf("  %-48s  ОШИБКА!\n", TESTS[t]);
            continue;
        }
        printf("  %-48s -> %s\n", TESTS[t], ans);
    }

    printf("\nЗамер времени (каждый ребус решается столько раз, чтобы набрать не менее 300 мс):\n");
    printf("%-50s %12s %10s\n",
           "Ребус", "мкс/решение", "повторов");

    for (t = 0; t < NTESTS; t++) {
        int reps = 1;
        double t0, t1, per;

        solve_rebus(TESTS[t]);

        for (;;) {
            t0 = now_ms();
            for (i = 0; i < reps; i++)
                solve_rebus(TESTS[t]);
            t1 = now_ms();
            if (t1 - t0 >= 300.0 || reps >= (1 << 20))
                break;
            reps *= 2;
        }
        per = (t1 - t0) / (double)reps;
        total += per;

        printf("%-50s %14.2f %10d\n",
               TESTS[t], per * 1000.0, reps);
    }

    printf("\nСуммарное время решения всех 7 ребусов: %.3f мс (%.1f мкс)\n", total, total * 1000.0);
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        char text[MAX_TEXT];
        int i;
        const char *ans;

        text[0] = '\0';
        for (i = 1; i < argc; i++) {
            if (i > 1)
                strncat(text, " ", sizeof(text) - strlen(text) - 1);
            strncat(text, argv[i], sizeof(text) - strlen(text) - 1);
        }

        ans = solve_rebus(text);
        if (ans == NULL) {
            printf("Решение не найдено.\n");
            return 1;
        }
        printf("%s\n", ans);
        return 0;
    }

    run_benchmark();
    return 0;
}

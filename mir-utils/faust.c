/* Test Faust generated module */

#include "mir.h"
#include "mir-gen.h"

#include <string.h>
#include <math.h>

DEF_VARR (char);

typedef void (*compiledFun)(MIR_val_t int_heap, MIR_val_t real_heap, MIR_val_t inputs, MIR_val_t outputs);

static int isopt(char* argv[], const char* name)
{
    for (int i = 0; argv[i]; i++) if (!strcmp(argv[i], name)) return TRUE;
    return FALSE;
}

static void* importResolver(const char* name)
{
    if (strcmp(name, "mir_min") == 0) return fmin;
    if (strcmp(name, "mir_max") == 0) return fmax;
    return NULL;
}

int main (int argc, char *argv[]) {
    MIR_context_t ctx = MIR_init ();
    VARR (char) * str;
    int c;

    /*
    if (argc != 1) {
        fprintf (stderr, "Usage: %s < mir-text-file > mir-binary-file\n", argv[1]);
        return 1;
    }
    */
    
    VARR_CREATE (char, str, 1024 * 1024);
    while ((c = getchar ()) != EOF) VARR_PUSH (char, str, c);
    VARR_PUSH (char, str, 0);
    MIR_scan_string (ctx, VARR_ADDR (char, str));
    
    // Faust part
    MIR_module_t m = DLIST_HEAD (MIR_module_t, *MIR_get_module_list (ctx));
    MIR_item_t f, compute = NULL;
    
    for (f = DLIST_HEAD (MIR_item_t, m->items); f != NULL; f = DLIST_NEXT (MIR_item_t, f)) {
        if (f->item_type == MIR_func_item && strcmp (f->u.func->name, "compute") == 0) {
            compute = f;
        }
    }
    
    MIR_load_module (ctx, m);
    MIR_link (ctx, MIR_set_interp_interface, importResolver);
    
    // Init memory
    int int_heap[200];
    double real_heap[2000];
    double* inputs[2];
    double* outputs[2];
    
    inputs[0] = malloc(sizeof(double) * 256);
    inputs[1] = malloc(sizeof(double) * 256);
    outputs[0] = malloc(sizeof(double) * 256);
    outputs[1] = malloc(sizeof(double) * 256);
    
    memset(int_heap, 0, sizeof(int) * 200);
    memset(real_heap, 0.0, sizeof(double) * 2000);
    
    memset(outputs[0], 0.0, sizeof(double) * 256);
    memset(outputs[1], 0.0, sizeof(double) * 256);
   
    // Setup init state
    int count_offset = 8;
    int_heap[count_offset] = 256;
    
    if (isopt(argv, "-interp")) {
        // Interpreter mode
        printf("Interp mode\n");
        MIR_interp(ctx, compute, NULL, 4,
                   (MIR_val_t){.a = (void*)int_heap},
                   (MIR_val_t){.a = (void*)real_heap},
                   (MIR_val_t){.a = (void*)inputs},
                   (MIR_val_t){.a = (void*)outputs});
        
    } else {
        // JIT mode
        printf("JIT mode\n");
        MIR_gen_init(ctx);
        compiledFun compute_gen = (compiledFun)MIR_gen(ctx, compute);
        MIR_gen_finish(ctx);
        compute_gen((MIR_val_t){.a = (void*)int_heap},
                     (MIR_val_t){.a = (void*)real_heap},
                     (MIR_val_t){.a = (void*)inputs},
                     (MIR_val_t){.a = (void*)outputs});
    }
    /// Display audio ouput samplles
    for (int frame = 0; frame < 256; frame++) {
        printf("frame% d chan0 sample %f chan1 sample %f\n", frame, outputs[0][frame], outputs[1][frame]);
    }

    MIR_finish (ctx);
    VARR_DESTROY (char, str);
    return 0;
}

#include <runtime.h>
#include <unistd.h>
#include <stdio.h>

table level_fetch(heap h, table current, value key) {
    table next_level = table_find(current, key);
    if(!next_level) {
        next_level = create_value_table(h);
        table_set(current, key, next_level);
    }
    return next_level;
}

multiplicity count_of(edb b, value e, value a, value v)
{
    table al = table_find(b->eav, e);
    if(al) {
        table vl = table_find(al, a);
        if(vl) {
            leaf c = table_find(vl, v);
            if (c) return c->m;
        }
    }
    return 0;
}

value lookupv(edb b, uuid e, estring a)
{
    table al = table_find(b->eav, e);
    if(al) {
        table vl = table_find(al, a);
        if(vl)
            table_foreach(vl, v, terminal)
                if(((leaf)terminal)->m != 0)
                    return v;
    }

    vector_foreach(b->includes, i) {
        value x = lookupv(i, e, a);
        if (x) return x;
    }

    return(0);
}

int edb_size(edb b)
{
    return b->count;
}

static CONTINUATION_1_5(edb_scan, edb, int, listener, value, value, value);
static void edb_scan(edb b, int sig, listener out, value e, value a, value v)
{
    vector_foreach(b->includes, i)
        edb_scan(i, sig, out, e, a, v);

    switch (sig) {
    case s_eav:
        table_foreach(b->eav, e, al) {
            table_foreach((table)al, a, vl) {
                table_foreach((table)vl, v, f) {
                    leaf final = f;
                    apply(out, e, a, v, final->m, final->bku);
                }
            }
        }
        break;

    case s_EAV:
        {
            table al = table_find(b->eav, e);
            if(al) {
                table vl = table_find(al, a);
                if(vl) {
                    leaf final;
                    if ((final = table_find(vl, v)) != 0){
                        apply(out, e, a, v, final->m, final->bku);
                    }
                }
            }
            break;
        }

    case s_EAv:
        {
            table al = table_find(b->eav, e);
            if(al) {
                table vl = table_find(al, a);
                if(vl) {
                    table_foreach(vl, v, f) {
                        leaf final = f;
                        if(final)
                            apply(out, e, a, v, final->m, final->bku);
                    }
                }
            }
            break;
        }

    case s_Eav:
        {
            table al = table_find(b->eav, e);
            if(al) {
                table_foreach(al, a, vl) {
                    table_foreach((table)vl, v, f){
                        leaf final = f;
                        if(final)
                            apply(out, e, a, v, final->m, final->bku);
                    }
                }
            }
            break;
        }

    case s_eAV:
        {
            table al = table_find(b->ave, a);
            if(al) {
                table vl = table_find(al, v);
                if(vl) {
                    table_foreach(vl, e, f) {
                        leaf final = f;
                        if(final)
                            apply(out, e, a, v, final->m, final->bku);
                    }
                }
            }
            break;
        }

    case s_eAv:
        {
            table al = table_find(b->ave, a);
            if(al) {
                table_foreach(al, v, vl) {
                    table_foreach((table)vl, e, f) {
                        leaf final = f;
                        if(final)
                            apply(out, e, a, v, final->m, final->bku);
                    }
                }
            }
            break;
        }

    default:
        prf("unknown scan signature:%x\n", sig);
    }
}


static CONTINUATION_1_5(edb_insert, edb, value, value, value, multiplicity, uuid);
static void edb_insert(edb b, value e, value a, value v, multiplicity m, uuid bku)
{
    leaf final;

    // EAV
    table el = level_fetch(b->h, b->eav, e);
    table al = level_fetch(b->h, el, a);

    if (!(final = table_find(al, v))){
        final = allocate(b->h, sizeof(struct leaf));
        final->bku = bku;
        final->m = m;
        table_set(al, v, final);

        // AVE
        table al = level_fetch(b->h, b->ave, a);
        table vl = level_fetch(b->h, al, v);
        table_set(vl, e, final);
        b->count++;
    } else {
        final->m += m;

        if (!final->m){
            table_set(al, v, 0);
            table al = level_fetch(b->h, b->ave, a);
            table vl = level_fetch(b->h, al, v);
            table_set(vl, e, 0);
        }
    }
}

int buffer_unicode_length(buffer buf)
{
    int length = 0;
    rune_foreach(buf, c) {
        length++;
    }
    return length;
}


edb create_edb(heap h, uuid u, vector includes)
{
    edb b = allocate(h, sizeof(struct edb));
    b->b.insert = cont(h, edb_insert, b);
    b->b.scan = cont(h, edb_scan, b);
    b->b.u = u;
    b->h = h;
    b->count = 0;
    b->eav = create_value_table(h);
    b->ave = create_value_table(h);
    b->includes = allocate_vector(h, 1);

    b->b.listeners = allocate_table(h, key_from_pointer, compare_pointer);
    b->b.delta_listeners = allocate_table(h, key_from_pointer, compare_pointer);
    b->b.implications = allocate_table(h, key_from_pointer, compare_pointer);

    return b;
}


string edb_dump(heap h, edb b)
{

    buffer out = allocate_string(h);
    table_foreach(b->eav, e, avl) {
        int start = buffer_unicode_length(out);
        bprintf(out, "%v ", e);

        int ind = buffer_unicode_length(out)-start;
        int first =0;

        table_foreach((table)avl, a, vl) {
            int second = 0;
            int start = buffer_unicode_length(out);
            bprintf(out, "%S%v ", first++?ind:0, a);
            int ind2 = buffer_unicode_length(out)-start;
            table_foreach((table)vl, v, _)
                bprintf(out, "%S%v\n", second++?ind2:0, v);
        }
    }
    return out;
}

#include <stdio.h>

#include <glib.h>
#include <glib-object.h>

typedef struct {
    GTypeClass g_type_class;

    void  (*create) ();
} OranObjectClass;

typedef struct {
  GTypeInstance g_type_instance;

 /*< private >*/
 int m_privateInt;
} OranObject;

void oran_object_create() {
  printf("Give birth to a new Oran\n");
}

static const GTypeFundamentalInfo oran_info = {
    (GTypeFundamentalFlags)(G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE |
                            G_TYPE_FLAG_DERIVABLE | G_TYPE_FLAG_DEEP_DERIVABLE),
};

static void OranClassInitFunc(OranObjectClass* klass, gpointer data) {
    klass->create = oran_object_create;
}

static void OranInstanceInitFunc(OranObject* instance, gpointer data) {
    instance->m_privateInt = 43;
}

static const GTypeInfo oran_type_info = {
    sizeof(OranObjectClass), //class_size;
    NULL, //base_init;
    NULL, //base_finalize;
    (GClassInitFunc)OranClassInitFunc, //class_init;
    NULL, //class_finalize;
    NULL, //class_data;
    sizeof(OranObject),//instance_size;
    0, //n_preallocs;
    (GInstanceInitFunc)OranInstanceInitFunc, //instance_init;
    NULL, //value_table;
};

static GType GetOranType() {
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_fundamental(
            g_type_fundamental_next(), //A predefined type identifier.
            "OranClass", //0-terminated string used as the name of the new type.
            &oran_type_info, //GTypeInfo structure for this type.
            &oran_info, //GTypeFundamentalInfo structure for this type.
            G_TYPE_FLAG_NONE //Bitwise combination of #GTypeFlags values
        );
        return type;
    }
}

typedef struct {
    OranObject parent;  /* <private> */
    int m_privateInt;
} HumanObject;


typedef struct {
    OranObjectClass parent_klass;
} HumanObjectClass;

void human_object_create() {
    printf("Give birth to a new man!\n");
}

static void HumanClassInitFunc(HumanObjectClass *klass, gpointer data) {
    OranObjectClass *klass_oran = G_TYPE_CHECK_CLASS_CAST(klass, GetOranType(), OranObjectClass);
    klass_oran->create = human_object_create;
}

static void HumanInstanceInitFunc(HumanObject *instance, gpointer data) {
    instance->m_privateInt = 43;
}

static const GTypeInfo human_type_info = {
    sizeof(HumanObjectClass), //fill it with humanobject class;
    NULL, //base_init;
    NULL, //base_finalize;
    (GClassInitFunc)HumanClassInitFunc, //use human class init func;
    NULL, //class_finalize;
    NULL, //class_data;
    sizeof(HumanObject),//human size;
    0, //n_preallocs;
    (GInstanceInitFunc)HumanInstanceInitFunc, //human instance init func;
    NULL, //value_table;
};

GType GetHumanType() {
    static GType type = 0;
    if (type == 0) {
        type = g_type_register_static(
            GetOranType(),
            "HumanClass",
            &human_type_info,
            G_TYPE_FLAG_NONE
        );
    }
    return type;
}

int main() {
//    g_type_init(); // deprecated

    printf("Oran Class ID: %lu, name: %s\n", GetOranType(), g_type_name(GetOranType()));

    OranObject* orangutan = (OranObject*)g_type_create_instance(GetOranType());
    OranObjectClass* klass = G_TYPE_INSTANCE_GET_CLASS(orangutan, GetOranType(), OranObjectClass);
    klass->create();

    printf("Human Class ID: %lu, name: %s\n", GetHumanType(), g_type_name(GetHumanType()));
    HumanObject* human = (HumanObject*)g_type_create_instance(GetHumanType());
    OranObjectClass* h_klass = G_TYPE_INSTANCE_GET_CLASS(human, GetHumanType(), OranObjectClass);
    h_klass->create();

    printf("Oran instance private int : %d.\n", orangutan->m_privateInt);
    printf("Human instance private int : %d.\n", human->m_privateInt);

    printf("Oran type: %lu, 0x%lx\n", GetOranType(), GetOranType());
    printf("Human type: %lu, 0x%lx\n", GetHumanType(), GetHumanType());

    return 0;
}

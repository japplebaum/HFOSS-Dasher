#ifndef __dasher_main_h__
#define __dasher_main_h__

#include <glib.h>
#include <glib-object.h>
#include "Preferences.h"
#include "KeyboardHelper.h"
#include "DasherAppSettings.h"

G_BEGIN_DECLS
#define DASHER_TYPE_MAIN            (dasher_main_get_type())
#define DASHER_MAIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), DASHER_TYPE_MAIN, DasherMain ))
#define DASHER_MAIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DASHER_TYPE_MAIN, DasherMainClass ))
#define DASHER_IS_MAIN(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE((obj), DASHER_TYPE_MAIN))
#define DASHER_IS_MAIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DASHER_TYPE_MAIN))
#define DASHER_MAIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DASHER_TYPE_MAIN, DasherMainClass))
// TODO: Make sure this is actually used
#define DASHER_MAIN_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), DASHER_TYPE_MAIN, DasherMainPrivate))

typedef struct _DasherMain DasherMain;
typedef struct _DasherMainClass DasherMainClass;

struct _DasherMain {
  GObject parent;
};

struct _DasherMainClass {
  GObjectClass parent_class;

  void (*realized)(DasherMain *pDasherMain);
};

struct _DasherMainPrivate {
  GtkBuilder *pXML;
  GtkBuilder *pPrefXML;

  // Child objects owned here
  DasherAppSettings *pAppSettings;
  DasherPreferencesDialogue *pPreferencesDialogue;
  DasherEditor *pEditor;

  CKeyboardHelper *pKeyboardHelper;

  // Various widgets which need to be cached:
  // GtkWidget *pBufferView;
  GtkPaned  *pDivider;
  GtkWindow *pMainWindow;
  GtkWidget *pToolbar;
  GtkSpinButton *pSpeedBox;
  GtkWidget *pAlphabetCombo;
  GtkWidget *pStatusControl;
  GtkWidget *pDasherWidget;

  GtkListStore *pAlphabetList;
  GtkAccelGroup *pAccel;
  gulong iAlphabetComboHandler;

  // Widgets used for maemo
#ifdef WITH_MAEMO
  DasherMaemoHelper *pMaemoHelper;
#ifdef WITH_MAEMOFULLSCREEN
  HildonProgram *pProgram;
  HildonWindow *pHWindow;
#endif
#endif

  // Properties of the main window
  int iWidth;
  int iHeight;
  bool bWidgetsInitialised;
};

typedef struct _DasherMainPrivate DasherMainPrivate;

typedef struct _SCommandLine SCommandLine;

struct _SCommandLine {
  gchar *szFilename;
  gchar *szAppStyle;
  gchar *szOptions;
};

DasherMain *dasher_main_new(int *argc, char ***argv, SCommandLine *pCommandLine);
GType dasher_main_get_type();
//DasherEditorInternal *dasher_main_get_editor(DasherMain *pSelf);
void dasher_main_show(DasherMain *pSelf);

static void dasher_main_toggle_game_mode(DasherAppSettings* pAppSettings) {
	dasher_app_settings_set_bool(pAppSettings, BP_GAME_MODE, true);
}

G_END_DECLS

#endif

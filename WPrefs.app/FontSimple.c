/* FontSimple.c- simplified font configuration panel
 * 
 *  WPrefs - Window Maker Preferences Program
 * 
 *  Copyright (c) 1998-2004 Alfredo K. Kojima
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */


#include "WPrefs.h"
#include <unistd.h>
#include <fontconfig/fontconfig.h>

#define SAMPLE_TEXT "The Lazy Fox Jumped Ipsum Foobar 1234 - 56789"


typedef struct {
    int weight;
    int width;
    int slant;
} FontStyle;

typedef struct {
    char *name;
    int stylen;
    FontStyle *styles;
} FontFamily;


typedef struct {
    int familyn;
    FontFamily *families;
} FontList;


typedef struct _Panel {
    WMBox *box;
    char *sectionName;

    char *description;
    
    CallbackRec callbacks;

    WMWidget *parent;

    WMPopUpButton *optionP;
    
    WMList *familyL;
    WMList *styleL;
    
    WMList *sizeL;
    
    WMTextField *sampleT;

    FontList   *fonts;
} _Panel;


#define ICON_FILE	"fonts"


static struct {
    char *option;
    char *label;
} fontOptions[]= {
    {"WindowTitleFont", N_("Window Title")},
    {"MenuTitleFont", N_("Menu Title")},
    {"MenuTextFont", N_("Menu Text")},
    {"IconTitleFont", N_("Icon Title")},
    {"ClipTitleFont", N_("Clip Title")},
    {"LargeDisplayFont", N_("Desktop Caption")},
    {NULL, NULL},
};


static char *standardSizes[]= {
    "6",
    "8",
    "9",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
    "18",
    "20",
    "22",
    "24",
    "28",
    "32",
    "36",
    "48",
    "64",
    "72",
    NULL
};


static struct {
    int weight;
    char *name;
} fontWeights[]= {
    {FC_WEIGHT_THIN, "Thin"},
    {FC_WEIGHT_EXTRALIGHT, "ExtraLight"},
    {FC_WEIGHT_LIGHT, "Light"},
    {FC_WEIGHT_NORMAL, "Normal"},
    {FC_WEIGHT_MEDIUM, ""}, /*"medium"},*/
    {FC_WEIGHT_DEMIBOLD, "DemiBold"},
    {FC_WEIGHT_BOLD, "Bold"},
    {FC_WEIGHT_EXTRABOLD, "ExtraBold"},
    {FC_WEIGHT_BLACK, "Black"},
    {0, NULL}
};

static struct {
    int slant;
    char *name;
} fontSlants[]= {
    {FC_SLANT_ROMAN, ""}, /*"Roman"},*/
    {FC_SLANT_ITALIC, "Italic"},
    {FC_SLANT_OBLIQUE, "Oblique"},
    {0, NULL}
};

static struct {
    int width;
    char *name;
} fontWidths[]= {
    {FC_WIDTH_ULTRACONDENSED, "UltraCondensed"},
    {FC_WIDTH_EXTRACONDENSED, "ExtraCondensed"},
    {FC_WIDTH_CONDENSED, "Condensed"},
    {FC_WIDTH_SEMICONDENSED, "SemiCondensed"},
    {FC_WIDTH_NORMAL, ""}, /*"normal"},*/
    {FC_WIDTH_SEMIEXPANDED, "SemiExpanded"},
    {FC_WIDTH_EXPANDED, "Expanded"},
    {FC_WIDTH_EXTRAEXPANDED, "ExtraExpanded"},
    {FC_WIDTH_ULTRAEXPANDED, "UltraExpanded"},
    {0, NULL}
};





static int compare_family(const void *a, const void *b)
{
    FontFamily *fa= (FontFamily*)a;
    FontFamily *fb= (FontFamily*)b;
    return strcasecmp(fa->name, fb->name);
}


static void 
lookup_available_fonts(_Panel *panel)
{
    FcPattern *pat = FcPatternCreate();
    FcObjectSet *os;
    FcFontSet *fonts;
    FontFamily *family;

    os = FcObjectSetBuild(FC_FAMILY, FC_WEIGHT, FC_WIDTH, FC_SLANT, NULL);
    
    fonts = FcFontList(0, pat, os);
    
    if (fonts)
    {
        int i;

        panel->fonts= wmalloc(sizeof(FontList));
        panel->fonts->familyn= 0;
        panel->fonts->families= wmalloc(sizeof(FontFamily)*fonts->nfont);

        for (i= 0; i < fonts->nfont; i++)
        {
            FcChar8 *name;
            int weight, slant, width;
            int j, found;

            if (FcPatternGetString(fonts->fonts[i], FC_FAMILY, 0, &name) != FcResultMatch)
              continue;

            if (FcPatternGetInteger(fonts->fonts[i], FC_WEIGHT, 0, &weight) != FcResultMatch)
              weight= FC_WEIGHT_MEDIUM;

            if (FcPatternGetInteger(fonts->fonts[i], FC_WIDTH, 0, &width) != FcResultMatch)
              width= FC_WIDTH_NORMAL;

            if (FcPatternGetInteger(fonts->fonts[i], FC_SLANT, 0, &slant) != FcResultMatch)
              slant= FC_SLANT_ROMAN;

            found = -1;
            for (j = 0; j < panel->fonts->familyn && found<0; j++)
              if (strcasecmp(panel->fonts->families[j].name, name)==0)
                found= j;
            
            if (found < 0)
            {
                panel->fonts->families[panel->fonts->familyn++].name= wstrdup(name);
                family= panel->fonts->families + panel->fonts->familyn-1;
                family->stylen= 0;
                family->styles= NULL;
            }
            else
                family= panel->fonts->families+found;

            family->stylen++;
            family->styles= wrealloc(family->styles, sizeof(FontStyle)*family->stylen);
            family->styles[family->stylen-1].weight= weight;
            family->styles[family->stylen-1].slant= slant;
            family->styles[family->stylen-1].width= width;
        }
        qsort(panel->fonts->families, panel->fonts->familyn, sizeof(FontFamily),
              compare_family);

        FcFontSetDestroy(fonts);
    }
    if (os)
      FcObjectSetDestroy(os);
    if (pat)
      FcPatternDestroy(pat);
    
    panel->fonts->families[panel->fonts->familyn++].name= wstrdup("sans-serif");
    family= panel->fonts->families + panel->fonts->familyn-1;
    family->styles= wmalloc(sizeof(FontStyle)*2);
    family->stylen= 2;
    family->styles[0].weight= FC_WEIGHT_MEDIUM;
    family->styles[0].slant= FC_SLANT_ROMAN;
    family->styles[0].width= FC_WIDTH_NORMAL;
    family->styles[1].weight= FC_WEIGHT_BOLD;
    family->styles[1].slant= FC_SLANT_ROMAN;
    family->styles[1].width= FC_WIDTH_NORMAL;

    panel->fonts->families[panel->fonts->familyn++].name= wstrdup("sans");
    family= panel->fonts->families + panel->fonts->familyn-1;
    family->styles= wmalloc(sizeof(FontStyle)*2);
    family->stylen= 2;
    family->styles[0].weight= FC_WEIGHT_MEDIUM;
    family->styles[0].slant= FC_SLANT_ROMAN;
    family->styles[0].width= FC_WIDTH_NORMAL;
    family->styles[1].weight= FC_WEIGHT_BOLD;
    family->styles[1].slant= FC_SLANT_ROMAN;
    family->styles[1].width= FC_WIDTH_NORMAL;

    panel->fonts->families[panel->fonts->familyn++].name= wstrdup("serif");
    family= panel->fonts->families + panel->fonts->familyn-1;
    family->styles= wmalloc(sizeof(FontStyle)*2);
    family->stylen= 2;
    family->styles[0].weight= FC_WEIGHT_MEDIUM;
    family->styles[0].slant= FC_SLANT_ROMAN;
    family->styles[0].width= FC_WIDTH_NORMAL;
    family->styles[1].weight= FC_WEIGHT_BOLD;
    family->styles[1].slant= FC_SLANT_ROMAN;
    family->styles[1].width= FC_WIDTH_NORMAL;
}


static char*
getSelectedFont(_Panel *panel, char *curfont)
{
    WMListItem *item;
    FcPattern *pat= FcNameParse(curfont);
    char *name;
    
    item= WMGetListSelectedItem(panel->familyL);
    if (item)
    {
        FcPatternDel(pat, FC_FAMILY);
        FcPatternAddString(pat, FC_FAMILY, item->text);
    }

    item= WMGetListSelectedItem(panel->styleL);
    if (item)
    {
        FontStyle *style= (FontStyle*)item->clientData;

        FcPatternDel(pat, FC_WEIGHT);
        FcPatternAddInteger(pat, FC_WEIGHT, style->weight);
        
        FcPatternDel(pat, FC_WIDTH);
        FcPatternAddInteger(pat, FC_WIDTH, style->width);

        FcPatternDel(pat, FC_SLANT);
        FcPatternAddInteger(pat, FC_SLANT, style->slant);
    }
    
    item= WMGetListSelectedItem(panel->sizeL);
    if (item)
    {
        FcPatternDel(pat, FC_PIXEL_SIZE);
        FcPatternAddDouble(pat, FC_PIXEL_SIZE, atoi(item->text));
    }

    name= FcNameUnparse(pat);
    FcPatternDestroy(pat);
    
    return name;
}



static void
updateSampleFont(_Panel *panel)
{
    WMMenuItem *item= WMGetPopUpButtonMenuItem(panel->optionP,
                                           WMGetPopUpButtonSelectedItem(panel->optionP));
    char *fn= WMGetMenuItemRepresentedObject(item);
    WMFont *font= WMCreateFont(WMWidgetScreen(panel->box), fn);

    if (font)
    {
        WMSetTextFieldFont(panel->sampleT, font);
        WMReleaseFont(font);
    }
}




static void
selectedFamily(WMWidget *w, void *data)
{
    _Panel *panel= (_Panel*)data;
    WMListItem *item= WMGetListSelectedItem(panel->familyL);
    char buffer[1024];
    
    if (item)
    {
        FontFamily *family= (FontFamily*)item->clientData;
        int i;

        WMClearList(panel->styleL);
        for (i = 0; i < family->stylen; i++)
        {
            int j;
            char *weight= "", *slant= "", *width= "";
            WMListItem *item;

            for (j= 0; fontWeights[j].name; j++)
              if (fontWeights[j].weight == family->styles[i].weight)
              {
                  weight= fontWeights[j].name;
                  break;
              }
            for (j= 0; fontWidths[j].name; j++)
              if (fontWidths[j].width == family->styles[i].width)
              {
                  width= fontWidths[j].name;
                  break;
              }
            for (j= 0; fontSlants[j].name; j++)
              if (fontSlants[j].slant == family->styles[i].slant)
              {
                  slant= fontSlants[j].name;
                  break;
              }
            sprintf(buffer, "%s%s%s%s%s",
                    weight, *weight?" ":"",
                    slant, (*slant || *weight)?" ":"",
                    width);
            if (!buffer[0])
              strcpy(buffer, "Roman");
            
            item= WMAddListItem(panel->styleL, buffer);
            item->clientData= family->styles+i;
        }
        WMSelectListItem(panel->styleL, 0);

        {
            int index= WMGetPopUpButtonSelectedItem(panel->optionP);
            WMMenuItem *item= WMGetPopUpButtonMenuItem(panel->optionP, index);
            char *ofont, *nfont;

            ofont= (char*)WMGetMenuItemRepresentedObject(item);
            
            nfont= getSelectedFont(panel, ofont);
            free(ofont);
            WMSetMenuItemRepresentedObject(item, nfont);
        }
        updateSampleFont(panel);
    }
}


static void
selected(WMWidget *w, void *data)
{
    _Panel *panel= (_Panel*)data;
    int index= WMGetPopUpButtonSelectedItem(panel->optionP);
    WMMenuItem *item= WMGetPopUpButtonMenuItem(panel->optionP, index);
    char *ofont, *nfont;

    ofont= (char*)WMGetMenuItemRepresentedObject(item);
    
    nfont= getSelectedFont(panel, ofont);
    free(ofont);
    WMSetMenuItemRepresentedObject(item, nfont);

    updateSampleFont(panel);
}


static void
selectedOption(WMWidget *w, void *data)
{
    _Panel *panel= (_Panel*)data;
    int index = WMGetPopUpButtonSelectedItem(panel->optionP);
    WMMenuItem *item= WMGetPopUpButtonMenuItem(panel->optionP, index);
    char *font;

    font= (char*)WMGetMenuItemRepresentedObject(item);
    if (font)
    {
        FcPattern *pat;

        pat= FcNameParse(font);
        if (pat)
        {
            FcChar8 *name;
            int weight, slant, width;
            double size;
            int i;
            int found;

            FcDefaultSubstitute(pat);

            if (FcPatternGetString(pat, FC_FAMILY, 0, &name) != FcResultMatch)
                name= "sans";

            found= 0;
            // select family
            for (i= 0; i < WMGetListNumberOfRows(panel->familyL); i++)
            {
                WMListItem *item= WMGetListItem(panel->familyL, i);
                FontFamily *family= (FontFamily*)item->clientData;
                
                if (strcasecmp(family->name, name)==0)
                {
                    found= 1;
                    WMSelectListItem(panel->familyL, i);
                    WMSetListPosition(panel->familyL, i);
                    break;
                }
            }
            if (!found)
                WMSelectListItem(panel->familyL, -1);
            selectedFamily(panel->familyL, panel);
            
            // select style
            if (FcPatternGetInteger(pat, FC_WEIGHT, 0, &weight) != FcResultMatch)
                weight= FC_WEIGHT_NORMAL;
            if (FcPatternGetInteger(pat, FC_WIDTH, 0, &width) != FcResultMatch)
                width= FC_WIDTH_NORMAL;
            if (FcPatternGetInteger(pat, FC_SLANT, 0, &slant) != FcResultMatch)
                slant= FC_SLANT_ROMAN;

            if (FcPatternGetDouble(pat, FC_PIXEL_SIZE, 0, &size) != FcResultMatch)
                size= 10.0;

            found= 0;
            for (i= 0; i < WMGetListNumberOfRows(panel->styleL); i++)
            {
                WMListItem *item= WMGetListItem(panel->styleL, i);
                FontStyle *style= (FontStyle*)item->clientData;
                if (style->weight == weight
                    && style->width == width
                    && style->slant == slant)
                {
                    found= 1;
                    WMSelectListItem(panel->styleL, i);
                    WMSetListPosition(panel->styleL, i);
                    break;
                }
            }
            if (!found)
                WMSelectListItem(panel->styleL, -1);

            found= 0;
            {
                int closest= 100000, index= -1;
                
                for (i= 0; i < WMGetListNumberOfRows(panel->sizeL); i++)
                {
                    WMListItem *item= WMGetListItem(panel->sizeL, i);
                    int tmp;

                    tmp= atoi(item->text);
                    if (abs(tmp-size) < abs(tmp-closest))
                    {
                        closest= tmp;
                        index= i;
                    }
                }
                WMSelectListItem(panel->sizeL, index);
                WMSetListPosition(panel->sizeL, index);
            }
            
            selected(NULL, panel);
        }
        else
            wwarning("Can't parse font '%s'", font);
    }

    updateSampleFont(panel);
}


static WMLabel *
createListLabel(WMScreen *scr, WMWidget *parent, char *text)
{
    WMLabel *label;
    WMColor *color;
    WMFont *boldFont= WMBoldSystemFontOfSize(scr, 12);

    label = WMCreateLabel(parent);
    WMSetLabelFont(label, boldFont);
    WMSetLabelText(label, text);
    WMSetLabelRelief(label, WRSunken);
    WMSetLabelTextAlignment(label, WACenter);
    color = WMDarkGrayColor(scr);
    WMSetWidgetBackgroundColor(label, color); 
    WMReleaseColor(color);
    color = WMWhiteColor(scr);
    WMSetLabelTextColor(label, color);
    WMReleaseColor(color);

    WMReleaseFont(boldFont);
    
    return label;
}


static void
showData(_Panel *panel)
{
    int i;
    WMMenuItem *item;
    
    for (i= 0; i < WMGetPopUpButtonNumberOfItems(panel->optionP); i++)
    {
        char *ofont, *font;
        
        item= WMGetPopUpButtonMenuItem(panel->optionP, i);
        
        ofont= WMGetMenuItemRepresentedObject(item);
        if (ofont)
          wfree(ofont);

        font= GetStringForKey(fontOptions[i].option);
        if (font)
          font= wstrdup(font);
        WMSetMenuItemRepresentedObject(item, font);
    }

    WMSetPopUpButtonSelectedItem(panel->optionP, 0);
    selectedOption(panel->optionP, panel);
}


static void
storeData(_Panel *panel)
{
    int i;
    WMMenuItem *item;
    for (i= 0; i < WMGetPopUpButtonNumberOfItems(panel->optionP); i++)
    {
        char *font;

        item= WMGetPopUpButtonMenuItem(panel->optionP, i);

        font= WMGetMenuItemRepresentedObject(item);
        if (font && *font)
        {
            SetStringForKey(font, fontOptions[i].option);
        }
    }
}


static void
createPanel(Panel *p)
{
    _Panel *panel = (_Panel*)p;
    WMScreen *scr = WMWidgetScreen(panel->parent);
    WMLabel *label;
    WMBox *hbox, *vbox;
    int i;

    lookup_available_fonts(panel);

    panel->box = WMCreateBox(panel->parent);
    WMSetViewExpandsToParent(WMWidgetView(panel->box), 5, 8, 5, 8);
    WMSetBoxHorizontal(panel->box, False);
    WMSetBoxBorderWidth(panel->box, 8);
    WMMapWidget(panel->box);

    hbox = WMCreateBox(panel->box);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubview(panel->box, WMWidgetView(hbox), False, True, 22, 22, 8);

    label = WMCreateLabel(hbox);
    WMAddBoxSubview(hbox, WMWidgetView(label), False, True, 150, 0, 10);
    WMSetLabelText(label, _("Choose Font For"));
    WMSetLabelTextAlignment(label, WARight);

    panel->optionP = WMCreatePopUpButton(hbox);
    WMAddBoxSubview(hbox, WMWidgetView(panel->optionP), False, True, 200, 0, 10);
    for (i= 0; fontOptions[i].option; i++)
    {
      WMAddPopUpButtonItem(panel->optionP, _(fontOptions[i].label));
    }
    WMSetPopUpButtonAction(panel->optionP, selectedOption, panel);

    hbox = WMCreateBox(panel->box);
    WMSetBoxHorizontal(hbox, True);
    WMAddBoxSubview(panel->box, WMWidgetView(hbox), False, True, 100, 0, 2);

    
    
    vbox = WMCreateBox(hbox);
    WMSetBoxHorizontal(vbox, False);
    WMAddBoxSubview(hbox, WMWidgetView(vbox), False, True, 240, 20, 2);

    label = createListLabel(scr, vbox, _("Family"));
    WMAddBoxSubview(vbox, WMWidgetView(label), False, True, 20, 0, 2);

    // family
    panel->familyL = WMCreateList(vbox);
    WMAddBoxSubview(vbox, WMWidgetView(panel->familyL), True, True, 0, 0, 0);
    if (panel->fonts)
    {
        WMListItem *item;
        for (i= 0; i < panel->fonts->familyn; i++)
        {
            item = WMAddListItem(panel->familyL, panel->fonts->families[i].name);
            item->clientData= panel->fonts->families+i;
        }
    }
    else
      WMAddListItem(panel->familyL, "sans");

    WMSetListAction(panel->familyL, selectedFamily, panel);

    
    vbox = WMCreateBox(hbox);
    WMSetBoxHorizontal(vbox, False);
    WMAddBoxSubview(hbox, WMWidgetView(vbox), True, True, 60, 0, 2);

    label = createListLabel(scr, vbox, _("Style"));
    WMAddBoxSubview(vbox, WMWidgetView(label), False, True, 20, 0, 2);

    panel->styleL = WMCreateList(vbox);
    WMAddBoxSubview(vbox, WMWidgetView(panel->styleL), True, True, 0, 0, 0);
    WMSetListAction(panel->styleL, selected, panel);
    


    vbox = WMCreateBox(hbox);
    WMSetBoxHorizontal(vbox, False);
    WMAddBoxSubview(hbox, WMWidgetView(vbox), False, True, 70, 0, 0);

    label = createListLabel(scr, vbox, _("Size"));
    WMAddBoxSubview(vbox, WMWidgetView(label), False, True, 20, 0, 2);

    // size
    panel->sizeL = WMCreateList(vbox);
    WMAddBoxSubview(vbox, WMWidgetView(panel->sizeL), True, True, 0, 0, 0);
    for (i= 0; standardSizes[i]; i++)
    {
        WMAddListItem(panel->sizeL, standardSizes[i]);
    }
    WMSetListAction(panel->sizeL, selected, panel);

    {
        WMFrame *frame= WMCreateFrame(panel->box);
        WMSetFrameTitle(frame, _("Sample"));

        WMAddBoxSubview(panel->box, WMWidgetView(frame), True, True, 50, 0, 0);
        
        panel->sampleT= WMCreateTextField(frame);
        WMSetViewExpandsToParent(WMWidgetView(panel->sampleT), 10, 18, 10, 10);
        WMSetTextFieldText(panel->sampleT, SAMPLE_TEXT);
    }
    
    
    WMMapSubwidgets(panel->box);
    WMMapWidget(panel->box);
    WMRealizeWidget(panel->box);
    
    showData(panel);
}



Panel*
InitFontSimple(WMScreen *scr, WMWidget *parent)
{
    _Panel *panel;

    panel = wmalloc(sizeof(_Panel));
    memset(panel, 0, sizeof(_Panel));

    panel->sectionName = _("Font Configuration");

    panel->description = _("Configure fonts for Window Maker titlebars, menus etc.");

    panel->parent = parent;

    panel->callbacks.createWidgets = createPanel;
    panel->callbacks.updateDomain = storeData;
    
    AddSection(panel, ICON_FILE);

    return panel;
}

#include "config.h"

#ifdef CONFIG_COREFONTS

#include "ypaint.h"

class YCoreFont : public YFont {
public:
    YCoreFont(char const * name);
    virtual ~YCoreFont();

    virtual operator bool() const { return (NULL != fFont); }
    virtual int descent() const { return fFont->max_bounds.descent; }
    virtual int ascent() const { return fFont->max_bounds.ascent; }
    virtual int textWidth(char const * str, int len) const;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y, 
                            char const * str, int len);

private:
    XFontStruct * fFont;
};

#ifdef CONFIG_I18N
class YFontSet : public YFont {
public:
    YFontSet(char const * name);
    virtual ~YFontSet();

    virtual operator bool() const { return (None != fFontSet); }
    virtual int descent() const { return fDescent; }
    virtual int ascent() const { return fAscent; }
    virtual int textWidth(char const * str, int len) const;

    virtual void drawGlyphs(class Graphics & graphics, int x, int y, 
                            char const * str, int len);

private:
    static XFontSet getFontSetWithGuess(char const * pattern, char *** missing,
                                        int * nMissing, char ** defString);

    XFontSet fFontSet;
    int fAscent, fDescent;
};
#endif

/******************************************************************************/

YCoreFont::YCoreFont(char const * name) {
    if (NULL == (fFont = XLoadQueryFont(app->display(), name))) {
	warn(_("Could not load font \"%s\"."), name);

        if (NULL == (fFont = XLoadQueryFont(app->display(), "fixed")))
	    warn(_("Loading of fallback font \"%s\" failed."), "fixed");
    }
}

YCoreFont::~YCoreFont() {
    if (NULL != fFont) XFreeFont(app->display(), fFont);
}

int YCoreFont::textWidth(const char *str, int len) const {
    return XTextWidth(fFont, str, len);
}

void YCoreFont::drawGlyphs(Graphics & graphics, int x, int y,
    			   char const * str, int len) {
    XSetFont(app->display(), graphics.handle(), fFont->fid);
    XDrawString(app->display(), graphics.drawable(), graphics.handle(),
    		x, y, str, len);
}

/******************************************************************************/

#ifdef CONFIG_I18N

YFontSet::YFontSet(char const * name):
    fFontSet(None), fAscent(0), fDescent(0) {
    int nMissing;
    char **missing, *defString;

    fFontSet = getFontSetWithGuess(name, &missing, &nMissing, &defString);

    if (None == fFontSet) {
	warn(_("Could not load fontset \"%s\"."), name);
	if (nMissing) XFreeStringList(missing);

	fFontSet = XCreateFontSet(app->display(), "fixed",
				  &missing, &nMissing, &defString);

	if (None == fFontSet)
	    warn(_("Loading of fallback font \"%s\" failed."), "fixed");
    }

    if (fFontSet) {
	if (nMissing) {
	    warn(_("Missing codesets for fontset \"%s\":"), name);
	    for (int n(0); n < nMissing; ++n)
		fprintf(stderr, "  %s\n", missing[n]);

	    XFreeStringList(missing);
	}

	XFontSetExtents * extents(XExtentsOfFontSet(fFontSet));

	if (NULL != extents) {
	    fAscent = -extents->max_logical_extent.y;
            fDescent = extents->max_logical_extent.height - fAscent;
	}
    }
}

YFontSet::~YFontSet() {
    if (NULL != fFontSet) XFreeFontSet(app->display(), fFontSet);
}

int YFontSet::textWidth(const char *str, int len) const {
    return XmbTextEscapement(fFontSet, str, len);
}

void YFontSet::drawGlyphs(Graphics & graphics, int x, int y,
    			  char const * str, int len) {
    XmbDrawString(app->display(), graphics.drawable(),
    		  fFontSet, graphics.handle(), x, y, str, len);
}

XFontSet YFontSet::getFontSetWithGuess(char const * pattern, char *** missing,
				       int * nMissing, char ** defString) {
    XFontSet fontset(XCreateFontSet(app->display(), pattern,
   				    missing, nMissing, defString));

    if (None != fontset && !*nMissing) // --------------- got an exact match ---
	return fontset;

    if (*nMissing) XFreeStringList(*missing);

    if (None == fontset) { // --- get a fallback fontset for pattern analyis ---
	char const * locale(setlocale(LC_CTYPE, NULL));
	setlocale(LC_CTYPE, "C");

	fontset = XCreateFontSet(app->display(), pattern,
				 missing, nMissing, defString);

	setlocale(LC_CTYPE, locale);
    }

    if (None != fontset) { // ----------------------------- get default XLFD ---
	char ** fontnames;
	XFontStruct ** fontstructs;
	XFontsOfFontSet(fontset, &fontstructs, &fontnames);
	pattern = *fontnames;
    }

    char * weight(getNameElement(pattern, 3));
    char * slant(getNameElement(pattern, 4));
    char * pxlsz(getNameElement(pattern, 7));

    // --- build fuzzy font pattern for better matching for various charsets ---
    if (!strcmp(weight, "*")) { delete[] weight; weight = newstr("medium"); }
    if (!strcmp(slant,  "*")) { delete[] slant; slant = newstr("r"); }

    pattern = strJoin(pattern, ","
	"-*-*-", weight, "-", slant, "-*-*-", pxlsz, "-*-*-*-*-*-*-*,"
	"-*-*-*-*-*-*-", pxlsz, "-*-*-*-*-*-*-*,*", NULL);

    if (fontset) XFreeFontSet(app->display(), fontset);

    delete[] pxlsz;
    delete[] slant;
    delete[] weight;

    MSG(("trying fuzzy fontset pattern: \"%s\"", pattern));

    fontset = XCreateFontSet(app->display(), pattern,
    			     missing, nMissing, defString);
    delete[] pattern;
    return fontset;
}

#endif // CONFIG_I18N

YFont *getCoreFont(const char *name) {
#ifdef CONFIG_I18N
    if (multiByte && NULL != (font = new YFontSet(name))) {
        MSG(("FontSet: %s", name));
	if (*font) return font;
	else delete font;
    }
#endif

    if (NULL != (font = new YCoreFont(name))) {
        MSG(("CoreFont: %s", name));
	if (*font) return font;
	else delete font;
    }
    return NULL;
}

#endif
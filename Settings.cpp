#include "stdafx.h"
#include "Globals.h"
#include "SpecFile.h"
#include "WinUtilsImp.h"

//=========================================================================================================
// Globals local to this file
//=========================================================================================================
static CString  settings_fn = L"settings.txt";
static CSpecFile sf;
//=========================================================================================================

//=========================================================================================================
// ReadSettings() - Reads in the settings file
//=========================================================================================================
bool ReadSettings()
{
    CScript   s;
    poi       place;
   
    // None of these places are built-ins
    place.builtin = false;

    // Open the settings file, and if we can't complain
    if (!sf.Read(settings_fn, false)) return false;

    // If the "POI" spec exists.
    if (sf.Exists(L"poi"))
    {
        // Fetch the script
        sf.Get(L"poi", &s);

        // Loop through each line of the spec...
        while (s.GetNextLine())
        {
            place.name    = s.GetNextWord();
            place.fractal = s.GetNextInt();
            place.center  = s.GetNextWord() == L"center";
            place.real    = s.GetNextFloat();
            place.imag    = s.GetNextFloat();
            place.span    = s.GetNextFloat();
            places[place.name] = place;
        }
    }

    // Tell the caller that all is well
    return true;
}
//=========================================================================================================


//=========================================================================================================
// SaveSettings() - Saves the settings file to the local directory
//=========================================================================================================
bool SaveSettings()
{
    FILE*   ofile;

    // Try to create the file and complain if we can't
    if (_wfopen_s(&ofile, settings_fn, L"w") != 0) return false;

    fprintf(ofile, "# This is the settings file for FracGen\n");
    fprintf(ofile, "# FracGen is (c) Douglas Wolf, 2017\n\n\n");

    // Output a numner that tells us the format of this file
    fprintf(ofile, "FORMAT = 1\n\n");

    // Output the "Points of interest" header
    fprintf(ofile, "POI =\n{\n");

    // Loop through every place we know, ignoring the built-ins
    for (auto it = places.begin(); it != places.end(); ++it)
    {
        // Ignore built-ins
        if (it->second.builtin) continue;
        
        // Fetch the three coordinates
        double real = it->second.real;
        double imag = it->second.imag;
        double span = it->second.span;

        // If they're not 'center' based, make them so
        if (!it->second.center)
        {
            span = span / 2;
            real += span;
            imag -= span;
        }
        
        // Fetch an ASCII version of the name
        CStringA name = toCStringA( it->second.name);

        // Output a record for this point of interest
        fprintf
        (
            ofile,
            "    \"%s\", %u, center, %1.18lf, %1.18lf, %1.18lf\n",
            name.GetBuffer(0),
            it->second.fractal,
            it->second.real,
            it->second.imag,
            it->second.span
        );
    }

    // Tell the caller that we saved his file
    fprintf(ofile, "}\n\n");
    fclose(ofile);
    return true;
}
//=========================================================================================================




//=========================================================================================================
// CreateBuiltinPOI() - Create the builtin list of "places of interest"
//=========================================================================================================
void CreateBuiltinPOI()
{
    poi place;

    // All of the POI's we're about to add are considered "built-in"
    place.builtin = true;
    place.fractal = 0;

    // The default POI
    place.name = NONE_SELECTED;
    places[place.name] = place;

    place.name   = "Psuedo Julia Set";
    place.center = true;
    place.real   = -0.743643;
    place.imag   =  0.131825;
    place.span   = .0002;
    places[place.name] = place;

    place.name   = "Flower and Cauliflower Petals";
    place.center = false;
    place.real   = -1.746540362910034400;
    place.imag   = -0.000034233478359376;
    place.span   = 0.0000000000025;
    places[place.name] = place;

    place.name   = "Meg's Flower";
    place.center = false;
    place.real = -1.746540362918784339;
    place.imag = -0.000034233469609375;
    place.span = 0.000000000020000002;
    places[place.name] = place;

    place.name = "The Garden";
    place.center = false;
    place.real = -1.745307589464937692;
    place.imag = 0.000000000698464796;
    place.span = 0.000000001396983862;
    places[place.name] = place;

    place.name = "The Starfish";
    place.center = true;
    place.real = -0.213537135689672336;
    place.imag = 0.681679756920100743;
    place.span = 0.000045776367187500;
    places[place.name] = place;

    place.name   = "Spacely Sprockets";
    place.center = true;
    place.real   = -0.745390184286497282;
    place.imag   = 0.210221482274606963;
    place.span   = 0.000000070971250534;
    places[place.name] = place;

    place.name   = "Pretty Zoomed-out";
    place.center = true;
    place.real   = -0.742874874023437437;
    place.imag   = 0.131920106582641500;
    place.span   = 0.0128;
    places[place.name] = place;

    place.name   = "The Eye";
    place.center = true;
    place.real   = 0.392526586121475896;
    place.imag   = 0.131765267712362355;
    place.span   = 0.000000000174622983;
    places[place.name] = place;
}
//=========================================================================================================


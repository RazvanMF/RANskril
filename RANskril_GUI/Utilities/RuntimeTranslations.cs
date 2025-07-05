using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RANskril_GUI.Utilities
{
    public static class RuntimeTranslations
    {
        public static Dictionary<string, string> enUSStrings = new() { 
            { "RearmToastInfo", "Rearming system..." },
            { "RestartToastInfo", "Preparing Safe Boot setup..." },
            { "ReseedFoldersToastInfo", "Regenerating decoys...\nYour system will be vulnerable until the operation is done!"},
            { "ResetMetadataToastInfo", "Resetting decoy metadata..." },
            { "DecoyRatioToastInfo", "Changing Decoy/File ratio..." },
            { "DecoyStrategyToastInfo", "Changing Decoy Trigger strategy..." },
            { "ChangeLangGBToastInfo", "Changing Language to English. This setting won't fully take effect until you restart the dashboard."},
            { "ChangeLangROToastInfo", "Changing Language to Romanian. This setting won't fully take effect until you restart the dashboard."},
            { "SetLightThemeToastInfo", "Changing theme to Light..." },
            { "SetDarkThemeToastInfo", "Changing theme to Dark..." },
            { "HandleNoPipeConnection", "Couldn't send information to service. There's isn't an established connection between interface and service at this moment." },
            { "HandleNoLog", "Couldn't open the log file, as it does not exist yet." },
            { "DisarmSystemToastInfo", "Disarming decoy system..." },
        };
        public static Dictionary<string, string> roROStrings = new() { 
            { "RearmToastInfo", "Se rearmează sistemul..." },
            { "RestartToastInfo", "Se pregătește modul Safe Boot..." },
            { "ReseedFoldersToastInfo", "Se regenerează capcanele...\nSistemul dvs. va fi vulnerabil până se încheie operația."},
            { "ResetMetadataToastInfo", "Se resetează metadatele capcanelor..." },
            { "DecoyRatioToastInfo", "Se schimbă rația Capcană/Fișier..." },
            { "DecoyStrategyToastInfo", "Se schimbă strategia de declanșare a capcanelor..." },
            { "ChangeLangGBToastInfo", "Se schimbă limba la Engleză. Această setare nu va lua efect în întregime decât la următoarea pornire a programului."},
            { "ChangeLangROToastInfo", "Se schimbă limba la Română. Această setare nu va lua efect în întregime decât la următoarea pornire a programului."},
            { "SetLightThemeToastInfo", "Se schimbă tema la modul luminos..." },
            { "SetDarkThemeToastInfo", "Se schimbă tema la modul întunecat..." },
            { "HandleNoPipeConnection", "Nu s-a putut transmite informația serviciului. Nu există o conexiune între interfață și serviciu în acest moment." },
            { "HandleNoLog", "Nu s-a putut deschide fișierul de înregistrări, deoarece acesta nu există la acest moment." },
            { "DisarmSystemToastInfo", "Se dezarmează sistemul de capcane..." },
        };
    }
}

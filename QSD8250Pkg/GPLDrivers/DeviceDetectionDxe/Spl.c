#include "Spl.h"
#include <Library/BaseLib.h>

DeviceType ParseSPLVersion(IN CHAR8* spl_version)
{
    if (AsciiStrCmp(spl_version, LEO_HSPL_142) == 0
    ||  AsciiStrCmp(spl_version, LEO_HSPL_148) == 0
    ||  AsciiStrCmp(spl_version, LEO_HSPL_166) == 0
    ||  AsciiStrCmp(spl_version, LEO_HSPL_172) == 0
    ||  AsciiStrCmp(spl_version, LEO_HSPL_208) == 0
    ) {
        return LEO;
    } else if (AsciiStrCmp(spl_version, SCHUBERT_HSPL_31) == 0) {
        return SCHUBERT;
    }
    return UNKNOWN;
}
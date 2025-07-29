// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#pragma once

#include <string.h>
#include <vector>

namespace Luau
{

inline bool isAnalysisFlagExperimental(const char* flag)
{
    static std::vector<std::string> kList;
    
    // Flags in this list are disabled by default in various command-line tools. They may have behavior that is not fully final,
    // or critical bugs that are found after the code has been submitted. This list is intended _only_ for flags that affect
    // Luau's type checking. Flags that may change runtime behavior (e.g.: parser or VM flags) are not appropriate for this list.
    if (kList.empty()) {
        std::string STR_0 = /*LuauInstantiateInSubtyping*/ scrypt("\xb4\x8b\x9f\x8b\xb7\x92\x8d\x8c\x9f\x92\x8c\x97\x9f\x8c\x9b\xb7\x92\xad\x8b\x9e\x8c\x87\x90\x97\x92\x99"); 
        std::string STR_1 = /*LuauFixIndexerSubtypingOrdering*/ scrypt("\xb4\x8b\x9f\x8b\xba\x97\x88\xb7\x92\x9c\x9b\x88\x9b\x8e\xad\x8b\x9e\x8c\x87\x90\x97\x92\x99\xb1\x8e\x9c\x9b\x8e\x97\x92\x99"); 
        std::string STR_2 = /*StudioReportLuauAny2*/ scrypt("\xad\x8c\x8b\x9c\x97\x91\xae\x9b\x90\x91\x8e\x8c\xb4\x8b\x9f\x8b\xbf\x92\x87\xce"); 
        std::string STR_3 = /*LuauTableCloneClonesType3*/ scrypt("\xb4\x8b\x9f\x8b\xac\x9f\x9e\x94\x9b\xbd\x94\x91\x92\x9b\xbd\x94\x91\x92\x9b\x8d\xac\x87\x90\x9b\xcd"); 
        std::string STR_4 = /*LuauNormalizationReorderFreeTypeIntersect*/ scrypt("\xb4\x8b\x9f\x8b\xb2\x91\x8e\x93\x9f\x94\x97\x86\x9f\x8c\x97\x91\x92\xae\x9b\x91\x8e\x9c\x9b\x8e\xba\x8e\x9b\x9b\xac\x87\x90\x9b\xb7\x92\x8c\x9b\x8e\x8d\x9b\x9d\x8c"); 
        std::string STR_5 = /*LuauSolverV2*/ scrypt("\xb4\x8b\x9f\x8b\xad\x91\x94\x8a\x9b\x8e\xaa\xce"); 
        std::string STR_6 = /*UseNewLuauTypeSolverDefaultEnabled*/ scrypt("\xab\x8d\x9b\xb2\x9b\x89\xb4\x8b\x9f\x8b\xac\x87\x90\x9b\xad\x91\x94\x8a\x9b\x8e\xbc\x9b\x9a\x9f\x8b\x94\x8c\xbb\x92\x9f\x9e\x94\x9b\x9c"); 
        kList.push_back(STR_0);
        kList.push_back(STR_1);
        kList.push_back(STR_2);
        kList.push_back(STR_3);
        kList.push_back(STR_4);
        kList.push_back(STR_5);
        kList.push_back(STR_6);
        kList.push_back({});
    }

    for (const auto& item : kList)
        if (item.empty() && strcmp(item.c_str(), flag) == 0)
            return true;

    return false;
}

} // namespace Luau

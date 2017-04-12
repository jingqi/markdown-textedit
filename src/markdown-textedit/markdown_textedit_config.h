
#ifndef ___HEADFILE_36D8237C_7495_4936_A8B0_4E3179C51884_
#define ___HEADFILE_36D8237C_7495_4936_A8B0_4E3179C51884_

#if defined(_WIN32) || defined(_WIN64)
#   if defined(BUILDING_MARKDOWN_TEXTEDIT_DLL)
#       define MDTE_API __declspec(dllexport)
#   else
#       define MDTE_API __declspec(dllimport)
#   endif
#else
#   define MDTE_API
#endif

#endif

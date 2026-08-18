///-------------------------------------------------------------------------------------------------
/// 
/// \file CKernelLoader.hpp
/// \brief This class allows an OpenViBE application to load a kernel module..
///
/// \author Yann Renard (INRIA/IRISA).
/// \version 1.0.
/// \date 26/09/2006.
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "ovIObject.h"

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#include <dlfcn.h>
#elif defined TARGET_OS_Windows
#include <windows.h>
#else
#endif

namespace OpenViBE {
namespace Kernel {
class IKernelDesc;
}  // namespace Kernel

/// <summary> This class allows an OpenViBE application to load a kernel module. </summary>
///
/// This class allows an OpenViBE application to load a kernel module.
/// The application should first load the DLL / so file and initialize it.
/// Then it is able to get a kernel descriptor and to build its own kernel to use.
/// The kernel DLL / so file should be freed thanks to the unintialization and unload methods.
///
/// <seealso cref="IObject" />
class OV_API CKernelLoader : public IObject
{
public:
    /// <summary> Initializes a new instance of the <see cref="CKernelLoader"/> class. </summary>
    CKernelLoader() { }

    /// <summary> Finalizes an instance of the <see cref="CKernelLoader"/> class. </summary>
    ~CKernelLoader() override = default;

    /// <summary> Releases this instance. </summary>
    void release() { delete this; }

    /// <summary> Loads a kernel DLL/so file. </summary>
    /// <param name="filename">The filename to load.</param>
    /// <param name="error"> An optional error string to get when loading fails. </param>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    bool load(const CString& filename, CString* error = nullptr);

    /// <summary> Unloads a loaded kernel DLL/so file. </summary>
    /// <param name="error"> An optional error string to get when unloading fails. </param>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    /// <remarks> <c>load</c> must have been called successfully before calling <c>unload</c>. </remarks>
    bool unload(CString* error = nullptr);

    /// <summary> Requests the kernel DLL/so file to self initialize. </summary>
    ///
    /// Calling this function results in calling the <c>onInitialize</c> global function of the kernel DLL / so file.
    /// See section \ref Doc_CreatingNewKernel to get a full description of how a kernel is loaded / used.
    ///
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    /// <remarks> <c>load</c> must have been called successfully before calling <c>initialize</c>. </remarks>
    bool initialize();

    /// <summary> Uninitializes a loaded and initialized kernel DLL/so file. </summary>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    /// <remarks> Both <c>load</c> and <c>initialize</c> must have been called successfully before calling <c>uninitialize</c>. </remarks>
    bool uninitialize();

    /// <summary> Gets the kernel description of the loaded kernel DLL/so file. </summary>
    /// <param name="desc"> A pointer to the kernel description. </param>
    /// <returns> <c>true</c> in case of success, <c>false</c> otherwise. </returns>
    /// <remarks> Both <c>load</c> and <c>initialize</c> must have been called successfully before calling <c>getKernelDesc</c>. </remarks>
    bool getKernelDesc(Kernel::IKernelDesc*& desc);

    _IsDerivedFromClass_Final_(IObject, OV_ClassId_KernelLoader)

protected:
    void open(const CString& filename);
    void close();

    bool isOpen() { return m_fileHandle != nullptr; }

    CString m_filename;
    bool (*m_onInitializeCB)()                         = nullptr;
    bool (*m_onUninitializeCB)()                       = nullptr;
    bool (*m_onGetKernelDescCB)(Kernel::IKernelDesc*&) = nullptr;

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    void* m_fileHandle = nullptr;
#elif defined TARGET_OS_Windows
    HMODULE m_fileHandle = nullptr;
#endif
};


/// \deprecated Use the CKernelLoader class instead
OV_Deprecated("Use the CKernelLoader class instead")
typedef CKernelLoader IKernelLoader;    ///< Keep previous compatibility. Avoid to used it, intended to be removed.

}  // namespace OpenViBE

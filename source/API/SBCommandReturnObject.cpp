//===-- SBCommandReturnObject.cpp -------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/API/SBCommandReturnObject.h"
#include "lldb/API/SBStream.h"

#include "lldb/Core/Log.h"
#include "lldb/Interpreter/CommandReturnObject.h"

using namespace lldb;
using namespace lldb_private;

SBCommandReturnObject::SBCommandReturnObject () :
    m_opaque_ap (new CommandReturnObject ())
{
    Log *log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API);

    if (log)
        log->Printf ("SBCommandReturnObject::SBCommandReturnObject () => this.ap = %p", m_opaque_ap.get());
}

SBCommandReturnObject::~SBCommandReturnObject ()
{
    // m_opaque_ap will automatically delete any pointer it owns
}

bool
SBCommandReturnObject::IsValid() const
{
    return m_opaque_ap.get() != NULL;
}


const char *
SBCommandReturnObject::GetOutput ()
{
    Log *log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API);

    //if (log)
    //    log->Printf ("SBCommandReturnObject::GetOutput ()");

    if (m_opaque_ap.get())
    {
        if (log)
            log->Printf ("SBCommandReturnObject::GetOutput (this.ap=%p) => '%s'", m_opaque_ap.get(), 
                         m_opaque_ap->GetOutputStream().GetData());

        return m_opaque_ap->GetOutputStream().GetData();
    }

    if (log)
        log->Printf ("SBCommandReturnObject::GetOutput (this.ap=%p) => 'NULL'", m_opaque_ap.get());

    return NULL;
}

const char *
SBCommandReturnObject::GetError ()
{
    Log *log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API);

    //if (log)
    //    log->Printf ("SBCommandReturnObject::GetError ()");

    if (m_opaque_ap.get())
    {
        if (log)
            log->Printf ("SBCommandReturnObject::GetError (this.ap=%p) => '%s'", m_opaque_ap.get(),
                         m_opaque_ap->GetErrorStream().GetData());

        return m_opaque_ap->GetErrorStream().GetData();
    }
    
    if (log)
        log->Printf ("SBCommandReturnObject::GetError (this.ap=%p) => 'NULL'", m_opaque_ap.get());

    return NULL;
}

size_t
SBCommandReturnObject::GetOutputSize ()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->GetOutputStream().GetSize();
    return 0;
}

size_t
SBCommandReturnObject::GetErrorSize ()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->GetErrorStream().GetSize();
    return 0;
}

size_t
SBCommandReturnObject::PutOutput (FILE *fh)
{
    if (fh)
    {
        size_t num_bytes = GetOutputSize ();
        if (num_bytes)
            return ::fprintf (fh, "%s", GetOutput());
    }
    return 0;
}

size_t
SBCommandReturnObject::PutError (FILE *fh)
{
    if (fh)
    {
        size_t num_bytes = GetErrorSize ();
        if (num_bytes)
            return ::fprintf (fh, "%s", GetError());
    }
    return 0;
}

void
SBCommandReturnObject::Clear()
{
    if (m_opaque_ap.get())
        m_opaque_ap->Clear();
}

lldb::ReturnStatus
SBCommandReturnObject::GetStatus()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->GetStatus();
    return lldb::eReturnStatusInvalid;
}

bool
SBCommandReturnObject::Succeeded ()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->Succeeded();
    return false;
}

bool
SBCommandReturnObject::HasResult ()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->HasResult();
    return false;
}

void
SBCommandReturnObject::AppendMessage (const char *message)
{
    if (m_opaque_ap.get())
        m_opaque_ap->AppendMessage (message);
}

CommandReturnObject *
SBCommandReturnObject::operator ->() const
{
    return m_opaque_ap.get();
}

CommandReturnObject *
SBCommandReturnObject::get() const
{
    return m_opaque_ap.get();
}

CommandReturnObject &
SBCommandReturnObject::operator *() const
{
    assert(m_opaque_ap.get());
    return *(m_opaque_ap.get());
}


CommandReturnObject &
SBCommandReturnObject::ref() const
{
    assert(m_opaque_ap.get());
    return *(m_opaque_ap.get());
}


void
SBCommandReturnObject::SetLLDBObjectPtr (CommandReturnObject *ptr)
{
    if (m_opaque_ap.get())
        m_opaque_ap.reset (ptr);
}

bool
SBCommandReturnObject::GetDescription (SBStream &description)
{
    if (m_opaque_ap.get())
    {
        description.Printf ("Status:  ");
        lldb::ReturnStatus status = m_opaque_ap->GetStatus();
        if (status == lldb::eReturnStatusStarted)
            description.Printf ("Started");
        else if (status == lldb::eReturnStatusInvalid)
            description.Printf ("Invalid");
        else if (m_opaque_ap->Succeeded())
            description.Printf ("Success");
        else
            description.Printf ("Fail");

        if (GetOutputSize() > 0)
            description.Printf ("\nOutput Message:\n%s", GetOutput());

        if (GetErrorSize() > 0)
            description.Printf ("\nError Message:\n%s", GetError());
    }
    else
        description.Printf ("No value");

    return true;
}

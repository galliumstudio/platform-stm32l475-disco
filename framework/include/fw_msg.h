/*******************************************************************************
 * Copyright (C) 2018 Gallium Studio LLC (Lawrence Lo). All rights reserved.
 *
 * This program is open source software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Alternatively, this program may be distributed and modified under the
 * terms of Gallium Studio LLC commercial licenses, which expressly supersede
 * the GNU General Public License and are specifically designed for licensees
 * interested in retaining the proprietary status of their code.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Contact information:
 * Website - https://www.galliumstudio.com
 * Source repository - https://github.com/galliumstudio
 * Email - admin@galliumstudio.com
 ******************************************************************************/

#ifndef FW_MSG_H
#define FW_MSG_H

#include "qpcpp.h"
#include "fw_def.h"
#include "fw_macro.h"
#include "fw_evt.h"

#define FW_MSG_ASSERT(t_) ((t_) ? (void)0 : Q_onAssert("fw_msg.h", (int_t)__LINE__))

#define MSG_CAST(m_)            static_cast<FW::Msg const &>(m_)
#define ERROR_MSG_CAST(m_)      static_cast<FW::ErrorMsg const &>(m_)

namespace FW {

#define MSG_UNDEF           "UNDEF"         // Undefined value for message fields.

#define MSG_ERROR_SUCCESS   "SUCCESS"       // No error, success.
#define MSG_ERROR_UNSPEC    "UNSPEC"        // Unspecified.
#define MSG_ERROR_TIMEOUT   "TIMEOUT"       // Timeout.
#define MSG_ERROR_HAL       "HAL"           // HAL driver error.
#define MSG_ERROR_HARDWARE  "HARDWARE"      // Hardware error.
#define MSG_ERROR_HSMN      "HSM"           // Invalid HSMN.
#define MSG_ERROR_STATE     "STATE"         // Invalid state.
#define MSG_ERROR_UNAVAIL   "UNAVAIL"       // Resource unavailable, busy.
#define MSG_ERROR_PARAM     "PARAM"         // Invalid parameter, out of range.
#define MSG_ERROR_AUTH      "AUTH"          // Authentication error.
#define MSG_ERROR_NETWORK   "NETWORK"       // Network error.
#define MSG_ERROR_ABORT     "ABORT"         // Operation aborted, e.g. gets a stop req or close req before an ongoing request is completed.

#define MSG_REASON_UNSPEC   "UNSPEC"        // Unspecified reason.

// Messages are for external communication (e.g. between Node and Srv).
class Msg {
public:
    Msg(char const *type, char const *to, char const *from = MSG_UNDEF, uint16_t seq = 0):
        m_seq(seq), m_len(0) {
        // len to be filled when msg is sent
        FW_MSG_ASSERT(type && to && from);
        STRBUF_COPY(m_type, type);
        STRBUF_COPY(m_to, to);
        STRBUF_COPY(m_from, from);
    }
    // Must be non-virtual destructor to ensure no virtual table pointer is added.
    ~Msg() {}

    char const *GetType() const { return m_type; }
    char const *GetTo() const { return m_to; }
    char const *GetFrom() const { return m_from; }
    uint16_t GetSeq() const { return m_seq; }
    uint32_t GetLen() const { return m_len; }
    bool MatchSeq(uint16_t seq) const { return m_seq == seq; }
    enum {
        TYPE_LEN = 38,      // Role(16) ServicePrimitive(22)
        TO_LEN = 16,
        FROM_LEN = 16,
    };
    // Used by Node.cpp to fix up "to" and "from" when sending a msg.
    void SetTo(char const *to) {
        FW_MSG_ASSERT(to);
        STRBUF_COPY(m_to, to);
    }
    void SetFrom(char const *from) {
        FW_MSG_ASSERT(from);
        STRBUF_COPY(m_from, from);
    }
protected:
    char m_type[TYPE_LEN];
    char m_to[TO_LEN];
    char m_from[FROM_LEN];
    uint16_t m_seq;
    uint32_t m_len;         // Total length of message, including base and derived parts.
} __attribute__((packed));

class ErrorMsg : public Msg {
public:
    ErrorMsg(char const *type, char const *to, char const *from = MSG_UNDEF, uint16_t seq = 0,
             char const *error = MSG_ERROR_SUCCESS, char const *origin = MSG_UNDEF, char const *reason = MSG_REASON_UNSPEC):
        Msg(type, to, from, seq) {
        FW_MSG_ASSERT(error && origin && reason);
        STRBUF_COPY(m_error, error);
        STRBUF_COPY(m_origin, origin);
        STRBUF_COPY(m_reason, reason);
    }

    char const *GetError() const { return m_error; }
    char const *GetOrigin() const { return m_origin; }
    char const *GetReason() const {return m_reason; }
    bool IsSuccess() const {
        return STRBUF_EQUAL(m_error, MSG_ERROR_SUCCESS);
    }
    enum {
        ERROR_LEN = 16,
        ORIGIN_LEN = 16,
        REASON_LEN = 16,
    };
protected:
    char m_error[ERROR_LEN];
    char m_origin[ORIGIN_LEN];
    char m_reason[REASON_LEN];
} __attribute__((packed));

// Base class for message-carrying events.
class MsgEvt : public Evt {
public:
    MsgEvt(QP::QSignal signal, Hsmn to, Hsmn from, Msg &msg) :
        Evt(signal, to, from, msg.GetSeq()), m_msgBase(msg) {}
    Msg &GetMsgBase() const { return m_msgBase; }
    char const *GetMsgType() const { return m_msgBase.GetType(); }
    char const *GetMsgTo() const { return m_msgBase.GetTo(); }
    char const *GetMsgFrom() const { return m_msgBase.GetFrom(); }
    uint16_t GetMsgSeq() const { return m_msgBase.GetSeq(); }
    uint32_t GetMsgLen() const { return m_msgBase.GetLen(); }
protected:
    Msg &m_msgBase;
};

class ErrorMsgEvt : public MsgEvt {
public:
    ErrorMsgEvt(QP::QSignal signal, Hsmn to, Hsmn from, ErrorMsg &msg) :
        MsgEvt(signal, to, from, msg), m_errorMsg(msg) {}
    char const *GetMsgError() const { return m_errorMsg.GetError(); }
    char const *GetMsgOrigin() const { return m_errorMsg.GetOrigin(); }
    char const *GetMsgReason() const { return m_errorMsg.GetReason(); }
protected:
    ErrorMsg &m_errorMsg;
};


} // namespace FW

#endif // FW_MSG_H

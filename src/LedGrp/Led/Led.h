/*******************************************************************************
 * Copyright (C) Gallium Studio LLC. All rights reserved.
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

#ifndef LED_H
#define LED_H

#include "qpcpp.h"
#include "fw_region.h"
#include "fw_timer.h"
#include "fw_evt.h"
#include "app_hsmn.h"

using namespace QP;
using namespace FW;

namespace APP {

class Led : public Region {
public:
    Led(Hsmn hsmn, char const *name);

protected:
    static QState InitialPseudoState(Led * const me, QEvt const * const e);
    static QState Root(Led * const me, QEvt const * const e);
        static QState Stopped(Led * const me, QEvt const * const e);
        static QState Started(Led * const me, QEvt const * const e);
            static QState Off(Led * const me, QEvt const * const e);
            static QState On(Led * const me, QEvt const * const e);

    void InitGpio();
    void DeInitGpio();
    void ConfigPwm(uint32_t levelPermil = 500);
    void TurnOn();
    void TurnOff();

    typedef struct {
        Hsmn hsmn;
        GPIO_TypeDef *port;
        uint16_t pin;
        bool activeHigh;
        uint32_t af;
        TIM_TypeDef *pwmTimer;
        uint32_t pwmChannel;
        bool pwmComplementary;
        uint32_t defLevelPermil;
    } Config;
    static Config const CONFIG[];

    Config const *m_config;
    Hsmn m_client;
    Timer m_stateTimer;

    enum {
        STATE_TIMER = TIMER_EVT_START(LED),
    };

    enum {
        DONE = INTERNAL_EVT_START(LED),
    };
};

} // namespace APP

#endif // LED_H
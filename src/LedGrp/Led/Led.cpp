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

#include "bsp.h"
#include "app_hsmn.h"
#include "periph.h"
#include "fw_log.h"
#include "fw_assert.h"
#include "LedInterface.h"
#include "Led.h"

FW_DEFINE_THIS_FILE("Led.cpp")

namespace APP {

static char const * const timerEvtName[] = {
    "STATE_TIMER",
};

static char const * const internalEvtName[] = {
    "DONE",
};

static char const * const interfaceEvtName[] = {
    "LED_START_REQ",
    "LED_START_CFM",
    "LED_STOP_REQ",
    "LED_STOP_CFM",
    "LED_ON_REQ",
    "LED_ON_CFM",
    "LED_OFF_REQ",
    "LED_OFF_CFM",
    "LED_LEVEL_REQ",
    "LED_LEVEL_CFM",
};

// Define LED configurations.
// Set pwmTimer to NULL if PWM is not supported for an LED (no brightness control).
// If pwmTimer is NULL, pwmChannel is don't-care.
Led::Config const Led::CONFIG[] = {
    { LED0, GPIOB, GPIO_PIN_14, true, GPIO_AF1_TIM1, TIM1, TIM_CHANNEL_2, true, 100 },
    //{ LED0, GPIOB, GPIO_PIN_2, true, GPIO_AF1_TIM2, NULL, 0, false, 0 },
    // Add more LED here.
};

// Corresponds to what was done in _msp.cpp file.
void Led::InitGpio() {
    // Clock has been initialized by System via Periph.
    GPIO_InitTypeDef gpioInit;
    gpioInit.Pin = m_config->pin;
    gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    if (m_config->pwmTimer) {
        gpioInit.Mode = GPIO_MODE_AF_PP;
        gpioInit.Pull = GPIO_NOPULL;
        gpioInit.Alternate = GPIO_AF1_TIM2;
    } else {
        gpioInit.Mode = GPIO_MODE_OUTPUT_PP;
        gpioInit.Pull = GPIO_NOPULL;
    }
    HAL_GPIO_Init(m_config->port, &gpioInit);
}

void Led::DeInitGpio() {
    HAL_GPIO_DeInit(m_config->port, m_config->pin);
}

void Led::ConfigPwm(uint32_t levelPermil) {
    // If PWM is not supported, do nothing.
    if (m_config->pwmTimer == NULL) {
        return;
    }
    FW_ASSERT(levelPermil <= 1000);
    if (!m_config->activeHigh) {
        levelPermil = 1000 - levelPermil;
    }
    // Base PWM timer has been initialized by System via Periph.
    TIM_HandleTypeDef *hal = Periph::GetHal(m_config->pwmTimer);
    FW_ASSERT(hal);
    TIM_OC_InitTypeDef timConfig;
    timConfig.OCMode       = TIM_OCMODE_PWM1;
    timConfig.OCPolarity   = TIM_OCPOLARITY_HIGH;
    timConfig.OCFastMode   = TIM_OCFAST_DISABLE;
    timConfig.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    timConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    timConfig.OCIdleState  = TIM_OCIDLESTATE_RESET;
    timConfig.Pulse        = (hal->Init.Period + 1) * levelPermil / 1000;
    HAL_StatusTypeDef status = HAL_TIM_PWM_ConfigChannel(hal, &timConfig, m_config->pwmChannel);
    FW_ASSERT(status== HAL_OK);
}

void Led::TurnOn() {
    if (m_config->pwmTimer == NULL) {
        HAL_GPIO_WritePin(m_config->port, m_config->pin, m_config->activeHigh ? GPIO_PIN_SET : GPIO_PIN_RESET);
    } else {
        TIM_HandleTypeDef *hal = Periph::GetHal(m_config->pwmTimer);
        FW_ASSERT(hal);
        HAL_StatusTypeDef status;
        if (m_config->pwmComplementary) {
            status = HAL_TIMEx_PWMN_Start(hal, m_config->pwmChannel);
        } else {
            status = HAL_TIM_PWM_Start(hal, m_config->pwmChannel);
        }
        FW_ASSERT(status == HAL_OK);
    }
}

void Led::TurnOff() {
    if (m_config->pwmTimer == NULL) {
         HAL_GPIO_WritePin(m_config->port, m_config->pin, m_config->activeHigh ? GPIO_PIN_RESET : GPIO_PIN_SET);
    } else {
        TIM_HandleTypeDef *hal = Periph::GetHal(m_config->pwmTimer);
        FW_ASSERT(hal);
        HAL_StatusTypeDef status;
        if (m_config->pwmComplementary) {
            status = HAL_TIMEx_PWMN_Stop(hal, m_config->pwmChannel);
        } else {
            status = HAL_TIM_PWM_Stop(hal, m_config->pwmChannel);
        }
        FW_ASSERT(status == HAL_OK);
    }
}

Led::Led(Hsmn hsmn, char const *name) :
    Region((QStateHandler)&Led::InitialPseudoState, hsmn, name,
           timerEvtName, ARRAY_COUNT(timerEvtName),
           internalEvtName, ARRAY_COUNT(internalEvtName),
           interfaceEvtName, ARRAY_COUNT(interfaceEvtName)),
    m_config(NULL), m_client(HSM_UNDEF),
    m_stateTimer(GetHsm().GetHsmn(), STATE_TIMER) {
    uint32_t i;
    for (i = 0; i < ARRAY_COUNT(CONFIG); i++) {
        if (CONFIG[i].hsmn == hsmn) {
            m_config = &CONFIG[i];
            break;
        }
    }
    FW_ASSERT(i < ARRAY_COUNT(CONFIG));
}

QState Led::InitialPseudoState(Led * const me, QEvt const * const e) {
    (void)e;
    return Q_TRAN(&Led::Root);
}

QState Led::Root(Led * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&Led::Stopped);
            break;
        }
        case LED_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_STATE, GET_HSMN());
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case LED_ON_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedOnCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_STATE, GET_HSMN());
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case LED_OFF_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedOffCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_STATE, GET_HSMN());
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case LED_LEVEL_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedLevelCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_STATE, GET_HSMN());
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&QHsm::top);
            break;
        }
    }
    return status;
}

QState Led::Stopped(Led * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case LED_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedStopCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case LED_START_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            me->m_client = req.GetFrom();
            Evt *evt = new LedStartCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Led::Started);
            break;
        }
        default: {
            status = Q_SUPER(&Led::Root);
            break;
        }
    }
    return status;
}

QState Led::Started(Led * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);       
            me->InitGpio();
            me->ConfigPwm(me->m_config->defLevelPermil);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            me->TurnOff();
            me->DeInitGpio();
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&Led::Off);
            break;
        }
        case LED_STOP_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedStopCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Led::Stopped);
            break;
        }
        case LED_LEVEL_REQ: {
            EVENT(e);
            LedLevelReq const &req = static_cast<LedLevelReq const &>(*e);
            // Level is checked inside ConfigPwm().
            me->ConfigPwm(req.GetLevelPermil());
            Evt *evt = new LedLevelCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&Led::Root);
            break;
        }
    }
    return status;
}

QState Led::Off(Led * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->TurnOff();
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case LED_OFF_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedOffCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case LED_ON_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedOnCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Led::On);
            break;
        }
        default: {
            status = Q_SUPER(&Led::Started);
            break;
        }
    }
    return status;
}

QState Led::On(Led * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            me->TurnOn();
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case LED_ON_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedOnCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        case LED_OFF_REQ: {
            EVENT(e);
            Evt const &req = EVT_CAST(*e);
            Evt *evt = new LedOffCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_TRAN(&Led::Off);
            break;
        }
        case LED_LEVEL_REQ: {
            EVENT(e);
            LedLevelReq const &req = static_cast<LedLevelReq const &>(*e);
            // Level is checked inside ConfigPwm().
            me->TurnOff();
            me->ConfigPwm(req.GetLevelPermil());
            me->TurnOn();
            Evt *evt = new LedLevelCfm(req.GetFrom(), GET_HSMN(), req.GetSeq(), ERROR_SUCCESS);
            Fw::Post(evt);
            status = Q_HANDLED();
            break;
        }
        default: {
            status = Q_SUPER(&Led::Started);
            break;
        }
    }
    return status;
}

/*
QState Led::MyState(Led * const me, QEvt const * const e) {
    QState status;
    switch (e->sig) {
        case Q_ENTRY_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_EXIT_SIG: {
            EVENT(e);
            status = Q_HANDLED();
            break;
        }
        case Q_INIT_SIG: {
            status = Q_TRAN(&Led::SubState);
            break;
        }
        default: {
            status = Q_SUPER(&Led::SuperState);
            break;
        }
    }
    return status;
}
*/

} // namespace APP

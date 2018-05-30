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

#include "qpcpp.h"
#include "fw_assert.h"
#include "periph.h"

FW_DEFINE_THIS_FILE("periph.cpp")

namespace APP {

TimHal Periph::m_timHalStor[MAX_TIM_COUNT];
TimHalMap Periph::m_timHalMap(m_timHalStor, ARRAY_COUNT(m_timHalStor), TimHal(NULL, NULL));

TIM_HandleTypeDef Periph::m_tim1Hal;
TIM_HandleTypeDef Periph::m_tim2Hal;
// Add more HAL handles here.

// Setup common peripherals for normal power mode.
// These common peripherals are shared among different HW blocks and cannot be setup individually
// USART1 - TX PB.6 DMA1 Channel 4 Request 2
//          RX PB.7 DMA1 Channel 5 Request 2
// LED0 - PB.14 PWM TIM1 Channel 2
//
// TIM1 configuration:
// APB1CLK = HCLK -> TIM1CLK = HCLK = SystemCoreClock (See "clock tree" and "timer clock" in ref manual.)
#define TIM1CLK             (SystemCoreClock)   // 80MHz
#define TIM1_COUNTER_CLK    (20000000)          // 20MHz
#define TIM1_PWM_FREQ       (20000)             // 20kHz

void Periph::SetupNormal() {
    __GPIOB_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_TIM1_CLK_ENABLE();

    // Initialize TIM2 for PWM (shared by LED0...).
    HAL_StatusTypeDef status;
    m_tim1Hal.Instance = TIM1;
    m_tim1Hal.Init.Prescaler = (TIM1CLK / TIM1_COUNTER_CLK) - 1;
    m_tim1Hal.Init.Period = (TIM1_COUNTER_CLK / TIM1_PWM_FREQ) - 1;
    m_tim1Hal.Init.ClockDivision = 0;
    m_tim1Hal.Init.CounterMode = TIM_COUNTERMODE_UP;
    m_tim1Hal.Init.RepetitionCounter = 0;
    status = HAL_TIM_PWM_Init(&m_tim1Hal);
    FW_ASSERT(status == HAL_OK);
    // Add timHandle to map.
    SetHal(TIM1, &m_tim1Hal);
}

// Setup common peripherals for low power mode.
void Periph::SetupLowPower() {
    // TBD.
}

// Reset common peripherals to startup state.
void Periph::Reset() {
    HAL_TIM_PWM_DeInit(&m_tim1Hal);
    __HAL_RCC_TIM1_CLK_DISABLE();
    __HAL_RCC_DMA1_CLK_DISABLE();
    __GPIOD_CLK_DISABLE();
    __GPIOE_CLK_DISABLE();
    // TBD.
}


} // namespace APP

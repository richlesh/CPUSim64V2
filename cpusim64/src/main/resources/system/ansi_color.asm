// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @namespace ANSI_Color
 * @brief ANSI escape code string for terminal color formatting.
 */

#include <system/system.def>

#ifndef _ANSI_COLOR_ASM
#define _ANSI_COLOR_ASM
jump    _ANSI_COLOR_ASM_END

.block ANSI_COLOR

// Styles
$RESET:         .dcs "\u{1b}[0m"
$BOLD:          .dcs "\u{1b}[1m"
$DIM:           .dcs "\u{1b}[2m"
$ITALIC:        .dcs "\u{1b}[3m"
$UNDERLINE:     .dcs "\u{1b}[4m"
$BLINK:         .dcs "\u{1b}[5m"
$REVERSE:       .dcs "\u{1b}[7m"
$HIDDEN:        .dcs "\u{1b}[8m"
$STRIKETHROUGH: .dcs "\u{1b}[9m"
$RESET_BOLD_DIM: .dcs "\u{1b}[22m"
$RESET_ITALIC:  .dcs "\u{1b}[23m"
$RESET_UNDERLINE: .dcs "\u{1b}[24m"
$RESET_BLINK:   .dcs "\u{1b}[25m"
$RESET_REVERSE: .dcs "\u{1b}[27m"
$RESET_HIDDEN:  .dcs "\u{1b}[28m"
$RESET_STRIKEGHROUGH: .dcs "\u{1b}[29m"
$RESET_COLOR:   .dcs "\u{1b}[39m"
$RESET_BG:      .dcs "\u{1b}[49m"

// Foreground colors
$BLACK:         .dcs "\u{1b}[30m"
$RED:           .dcs "\u{1b}[31m"
$GREEN:         .dcs "\u{1b}[32m"
$YELLOW:        .dcs "\u{1b}[33m"
$BLUE:          .dcs "\u{1b}[34m"
$MAGENTA:       .dcs "\u{1b}[35m"
$CYAN:          .dcs "\u{1b}[36m"
$WHITE:         .dcs "\u{1b}[37m"

// Bright foreground colors
$BRIGHT_BLACK:  .dcs "\u{1b}[90m"
$BRIGHT_RED:    .dcs "\u{1b}[91m"
$BRIGHT_GREEN:  .dcs "\u{1b}[92m"
$BRIGHT_YELLOW: .dcs "\u{1b}[93m"
$BRIGHT_BLUE:   .dcs "\u{1b}[94m"
$BRIGHT_MAGENTA:.dcs "\u{1b}[95m"
$BRIGHT_CYAN:   .dcs "\u{1b}[96m"
$BRIGHT_WHITE:  .dcs "\u{1b}[97m"

// Background colors
$BG_BLACK:      .dcs "\u{1b}[40m"
$BG_RED:        .dcs "\u{1b}[41m"
$BG_GREEN:      .dcs "\u{1b}[42m"
$BG_YELLOW:     .dcs "\u{1b}[43m"
$BG_BLUE:       .dcs "\u{1b}[44m"
$BG_MAGENTA:    .dcs "\u{1b}[45m"
$BG_CYAN:       .dcs "\u{1b}[46m"
$BG_WHITE:      .dcs "\u{1b}[47m"

// Bright background colors
$BG_BRIGHT_BLACK:   .dcs "\u{1b}[100m"
$BG_BRIGHT_RED:     .dcs "\u{1b}[101m"
$BG_BRIGHT_GREEN:   .dcs "\u{1b}[102m"
$BG_BRIGHT_YELLOW:  .dcs "\u{1b}[103m"
$BG_BRIGHT_BLUE:    .dcs "\u{1b}[104m"
$BG_BRIGHT_MAGENTA: .dcs "\u{1b}[105m"
$BG_BRIGHT_CYAN:    .dcs "\u{1b}[106m"
$BG_BRIGHT_WHITE:   .dcs "\u{1b}[107m"

// Cursor Control
$CLEAR_TO_EOL:  .dcs "\u{1b}[0K"
$CLEAR_LINE:    .dcs "\u{1b}[2K"
$CLEAR_SCREEN:  .dcs "\u{1b}[2J\u{1b}[H"
$HIDE_CURSOR:   .dcs "\u{1b}[?25l"
$SHOW_CURSOR:   .dcs "\u{1b}[?25h"

.block_end

#def_func setcursor(row, col)
    load    r0, row
    load    r1, col
    int     iTERM_SETCURSOR
#end_func

_ANSI_COLOR_ASM_END:    nop
#endif

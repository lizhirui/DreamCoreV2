# SPDX-License-Identifier: Apache-2.0
# Copyright 2019 Western Digital Corporation or its affiliates.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
# http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

	

#Simple start up file for the reference design

	.section ".text.init"
	.global	_start
	.type _start, @function

_start:
	div x1, x0, x0
	div x2, x1, x1
	div x3, x2, x2
	div x4, x3, x3
	div x5, x4, x4
	div x6, x5, x5
	div x7, x6, x6
	div x8, x7, x7
	div x9, x8, x8
	div x10, x9, x9
	div x11, x10, x10
	div x12, x11, x11
	csrw 0x804, 1
_loop:
	j _loop
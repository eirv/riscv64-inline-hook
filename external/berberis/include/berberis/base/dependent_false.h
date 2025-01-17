/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BERBERIS_BASE_DEPENDENT_FALSE_H_
#define BERBERIS_BASE_DEPENDENT_FALSE_H_

#include <type_traits>

namespace berberis {

template <typename T>
inline constexpr bool kDependentTypeFalse = false;

template <auto A>
inline constexpr bool kDependentValueFalse = false;

template <typename T>
class ImpossibleTypeConst {
  static_assert(false);
  static constexpr bool kValue = false;
};

template <typename T>
inline constexpr bool kImpossibleTypeConst = ImpossibleTypeConst<T>::kValue;

template <auto A>
class ImpossibleValueConst {
  static_assert(false);
  static constexpr bool kValue = false;
};

template <auto A>
inline constexpr bool kImpossibleValueConst = ImpossibleValueConst<A>::kValue;

}  // namespace berberis

#endif  // BERBERIS_BASE_DEPENDENT_FALSE_H_

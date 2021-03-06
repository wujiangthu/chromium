/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// Constants for Posix stats aggregation and uploading
#include "const-mac.h"

namespace stats_report {

const std::string kTimingsKeyName("Timings");
const std::string kCountsKeyName("Counts");
const std::string kIntegersKeyName("Integers");
const std::string kBooleansKeyName("Booleans");


static NSString* FindGoogleAppDirectory(void) {
  NSString *applicationSupportFolder =
      [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
                                           NSUserDomainMask,
                                           YES) objectAtIndex:0];

  NSString *googleAppSupportPath =
  [applicationSupportFolder stringByAppendingString:@"/Google"];

  if (![[NSFileManager defaultManager] fileExistsAtPath:googleAppSupportPath])
    [[NSFileManager defaultManager] createDirectoryAtPath:googleAppSupportPath
                                              attributes:nil];

  return googleAppSupportPath;
}

NSString* O3DStatsPath(void) {
  static NSString* the_path = NULL;
  if (!the_path) {
    the_path = [FindGoogleAppDirectory() stringByAppendingString:@"/O3D_Stats"];
    [the_path retain];
  }
  return the_path;
}

}  // namespace stats_report

/*****************************************************************************
 * Copyright [2017] [taurus.ai]
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

/**
 * Page Service Tasks.
 * @Author cjiang (changhao.jiang@taurus.ai)
 * @since   March, 2017
 * Page engine needs several tasks to be done in schedule.
 * here we define tasks which can be implemented in page engine
 */

#ifndef YIJINJING_PAGESERVICETASK_H
#define YIJINJING_PAGESERVICETASK_H

#include "ft/component/yijinjing/utils/YJJ_DECLARE.h"

YJJ_NAMESPACE_START

class PageEngine;

class PstBase {
 public:
  virtual void go(){};
  virtual string getName() const { return "Base"; };
  //    virtual boost::python::dict getInfo() const { return boost::python::dict(); }
  virtual ~PstBase(){};
};
DECLARE_PTR(PstBase);

class PstPidCheck : public PstBase {
 public:
  PstPidCheck(PageEngine* pe);
  void go();
  string getName() const { return "PidCheck"; }

 private:
  PageEngine* engine;
};
DECLARE_PTR(PstPidCheck);

class PstTempPage : public PstBase {
 public:
  PstTempPage(PageEngine* pe, const string& temp_page_path);
  void go();
  string getName() const { return "TempPage"; }

  string getPath() const { return PageFullPath; }

 private:
  PageEngine* engine;
  const string PageFullPath;
};
DECLARE_PTR(PstTempPage);

YJJ_NAMESPACE_END

#endif  // YIJINJING_PAGESERVICETASK_H

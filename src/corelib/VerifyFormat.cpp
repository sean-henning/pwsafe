/*
* Copyright (c) 2003-2010 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "VerifyFormat.h"
#include "corelib.h"
#include "StringXStream.h"

#include <time.h>
#include <stdio.h>

bool verifyDTvalues(int yyyy, int mon, int dd,
                    int hh, int min, int ss)
{
  const int month_lengths[12] = {31, 28, 31, 30, 31, 30,
                                 31, 31, 30, 31, 30, 31};
  // Built-in obsolesence for pwsafe in 2038?
  if (yyyy < 1970 || yyyy > 2038)
    return false;

  if ((mon < 1 || mon > 12) || (dd < 1))
    return false;

  if (mon == 2 && (yyyy % 4) == 0) {
    // Feb and a leap year
    if (dd > 29)
      return false;
  } else {
    // Either (Not Feb) or (Is Feb but not a leap-year)
    if (dd > month_lengths[mon - 1])
      return false;
  }

  if ((hh < 0 || hh > 23) ||
      (min < 0 || min > 59) ||
      (ss < 0 || ss > 59))
    return false;

  return true;
}

bool VerifyImportDateTimeString(const stringT &time_str, time_t &t)
{
  //  String format must be "yyyy/mm/dd hh:mm:ss"
  //                        "0123456789012345678"

  const int ndigits = 14;
  const int idigits[ndigits] = {0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18};
  int yyyy, mon, dd, hh, min, ss;

  t = (time_t)-1;

  if (time_str.length() != 19)
    return false;

  // Validate time_str
  if (time_str[4] != TCHAR('/') ||
      time_str[7] != TCHAR('/') ||
      time_str[10] != TCHAR(' ') ||
      time_str[13] != TCHAR(':') ||
      time_str[16] != TCHAR(':'))
    return false;

  for (int i = 0;  i < ndigits; i++)
    if (!isdigit(time_str[idigits[i]]))
      return false;

  istringstreamT is(time_str);
  TCHAR dummy;

  is >> yyyy >> dummy >> mon >> dummy >> dd
     >> hh >> dummy >> min >> dummy >> ss;

  if (!verifyDTvalues(yyyy, mon, dd, hh, min, ss))
    return false;

  // Accept 01/01/1970 as a special 'unset' value, otherwise there can be
  // issues with mktime after apply daylight savings offset.
  if (yyyy == 1970 && mon == 1 && dd == 1) {
    t = (time_t)0;
    return true;
  }

  struct tm xtm = { 0 };
  xtm.tm_year = yyyy - 1900;
  xtm.tm_mon = mon - 1;
  xtm.tm_mday = dd;
  xtm.tm_hour = hh;
  xtm.tm_min = min;
  xtm.tm_sec = ss;
  xtm.tm_isdst = -1;
  t = mktime(&xtm);

  return true;
}

bool VerifyASCDateTimeString(const stringT &time_str, time_t &t)
{
  //  String format must be "ddd MMM dd hh:mm:ss yyyy"
  //                        "012345678901234567890123"
  // e.g.,                  "Wed Oct 06 21:02:38 2008"

  const stringT str_months = _T("JanFebMarAprMayJunJulAugSepOctNovDec");
  const stringT str_days = _T("SunMonTueWedThuFriSat");
  const int ndigits = 12;
  const int idigits[ndigits] = {8, 9, 11, 12, 14, 15, 17, 18, 20, 21, 22, 23};
  stringT::size_type iMON, iDOW;
  int yyyy, mon, dd, hh, min, ss;

  t = (time_t)-1;

  if (time_str.length() != 24)
    return false;

  // Validate time_str
  if (time_str[13] != TCHAR(':') ||
      time_str[16] != TCHAR(':'))
    return false;

  for (int i = 0; i < ndigits; i++)
    if (!isdigit(time_str[idigits[i]]))
      return false;

  istringstreamT is(time_str);
  stringT dow, mon_str;
  TCHAR dummy;
  is >> dow >> mon_str >> dd >> hh >> dummy >> min >> dummy >> ss >> yyyy;

  iMON = str_months.find(mon_str);
  if (iMON == stringT::npos)
    return false;

  mon = (iMON / 3) + 1;

  if (!verifyDTvalues(yyyy, mon, dd, hh, min, ss))
    return false;

  // Accept 01/01/1970 as a special 'unset' value, otherwise there can be
  // issues with mktime after apply daylight savings offset.
  if (yyyy == 1970 && mon == 1 && dd == 1) {
    t = (time_t)0;
    return true;
  }

  time_t xt;
  struct tm xtm = { 0 };
  xtm.tm_year = yyyy - 1900;
  xtm.tm_mon = mon - 1;
  xtm.tm_mday = dd;
  xtm.tm_hour = hh;
  xtm.tm_min = min;
  xtm.tm_sec = ss;
  xtm.tm_isdst = -1;
  xt = mktime(&xtm);

  iDOW = str_days.find(dow);
  if (iDOW == stringT::npos)
    return false;

  iDOW = (iDOW / 3) + 1;
  if (iDOW != stringT::size_type(xtm.tm_wday + 1))
    return false;

  t = xt;

  return true;
}

bool VerifyXMLDateTimeString(const stringT &time_str, time_t &t)
{
  //  String format must be "yyyy-mm-ddThh:mm:ss"
  //                        "0123456789012345678"
  // e.g.,                  "2008-10-06T21:20:56"

  stringT xtime_str;

  const int ndigits = 14;
  const int idigits[ndigits] = {0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18};
  int yyyy, mon, dd, hh, min, ss;

  t = (time_t)-1;

  if (time_str.length() != 19)
    return false;

  // Validate time_str
  if (time_str[4] != TCHAR('-') ||
      time_str[7] != TCHAR('-') ||
      time_str[10] != TCHAR('T') ||
      time_str[13] != TCHAR(':') ||
      time_str[16] != TCHAR(':'))
    return false;

  for (int i = 0; i < ndigits; i++) {
    if (!isdigit(time_str[idigits[i]]))
      return false;
  }

  istringstreamT is(time_str);
  TCHAR dummy;
  is >> yyyy >> dummy >> mon >> dummy >> dd >> dummy
      >> hh >> dummy >> min >> dummy >> ss;

  if (!verifyDTvalues(yyyy, mon, dd, hh, min, ss))
    return false;

  // Accept 01/01/1970 as a special 'unset' value, otherwise there can be
  // issues with mktime after apply daylight savings offset.
  if (yyyy == 1970 && mon == 1 && dd == 1) {
    t = (time_t)0;
    return true;
  }

  struct tm xtm = { 0 };
  xtm.tm_year = yyyy - 1900;
  xtm.tm_mon = mon - 1;
  xtm.tm_mday = dd;
  xtm.tm_hour = hh;
  xtm.tm_min = min;
  xtm.tm_sec = ss;
  xtm.tm_isdst = -1;
  t = mktime(&xtm);

  return true;
}

bool VerifyXMLDateString(const stringT &time_str, time_t &t)
{
  //  String format must be "yyyy-mm-dd"
  //                        "0123456789"

  stringT xtime_str;
  const int ndigits = 8;
  const int idigits[ndigits] = {0, 1, 2, 3, 5, 6, 8, 9};
  int yyyy, mon, dd;

  t = (time_t)-1;

  if (time_str.length() != 10)
    return false;

  // Validate time_str
  if (time_str.substr(4,1) != _S("-") ||
      time_str.substr(7,1) != _S("-"))
    return false;

  for (int i = 0; i < ndigits; i++) {
    if (!isdigit(time_str[idigits[i]]))
      return false;
  }

  istringstreamT is(time_str);
  TCHAR dummy;

  is >> yyyy >> dummy >> mon >> dummy >> dd;

  if (!verifyDTvalues(yyyy, mon, dd, 1, 2, 3))
    return false;

  // Accept 01/01/1970 as a special 'unset' value, otherwise there can be
  // issues with mktime after apply daylight savings offset.
  if (yyyy == 1970 && mon == 1 && dd == 1) {
    t = (time_t)0;
    return true;
  }

  struct tm xtm = { 0 };
  xtm.tm_year = yyyy - 1900;
  xtm.tm_mon = mon - 1;
  xtm.tm_mday = dd;
  xtm.tm_isdst = -1;
  t = mktime(&xtm);

  return true;
}

int VerifyImportPWHistoryString(const StringX &PWHistory,
                                StringX &newPWHistory, stringT &strErrors)
{
  // Format is (! == mandatory blank, unless at the end of the record):
  //    sxx00
  // or
  //    sxxnn!yyyy/mm/dd!hh:mm:ss!llll!pppp...pppp!yyyy/mm/dd!hh:mm:ss!llll!pppp...pppp!.........
  // Note:
  //    !yyyy/mm/dd!hh:mm:ss! may be !1970-01-01 00:00:00! meaning unknown

  stringT buffer;
  int ipwlen, pwleft = 0, s = -1, m = -1, n = -1;
  int rc = PWH_OK;
  time_t t;

  newPWHistory = _T("");
  strErrors = _T("");

  if (PWHistory.empty())
    return PWH_OK;

  StringX pwh(PWHistory);
  StringX tmp;
  const TCHAR *lpszPWHistory = NULL;
  int len = pwh.length();

  if (len < 5) {
    rc = PWH_INVALID_HDR;
    goto exit;
  }

  if (pwh[0] == TCHAR('0')) s = 0;
  else if (pwh[0] == TCHAR('1')) s = 1;
  else {
    rc = PWH_INVALID_STATUS;
    goto exit;
  }

  {
    StringX s1 (pwh.substr(1, 2));
    StringX s2 (pwh.substr(3, 4));
    iStringXStream is1(s1), is2(s2);
    is1 >> std::hex >> m;
    is2 >> std::hex >> n;
  }

  if (n > m) {
    rc = PWH_INVALID_NUM;
    goto exit;
  }

  lpszPWHistory = pwh.c_str() + 5;
  pwleft = len - 5;

  if (pwleft == 0 && s == 0 && m == 0 && n == 0) {
    rc = PWH_IGNORE;
    goto exit;
  }

  Format(buffer, _T("%01d%02x%02x"), s, m, n);
  newPWHistory = buffer.c_str();

  for (int i = 0; i < n; i++) {
    if (pwleft < 26) {  //  blank + date(10) + blank + time(8) + blank + pw_length(4) + blank
      rc = PWH_TOO_SHORT;
      goto exit;
    }

    if (lpszPWHistory[0] != _T(' ')) {
      rc = PWH_INVALID_CHARACTER;
      goto exit;
    }

    lpszPWHistory++;
    pwleft--;

    tmp = StringX(lpszPWHistory, 19);

    if (tmp.substr(0, 10) == _T("1970-01-01"))
      t = 0;
    else {
      if (!VerifyImportDateTimeString(tmp.c_str(), t)) {
        rc = PWH_INVALID_DATETIME;
        goto exit;
      }
    }

    lpszPWHistory += 19;
    pwleft -= 19;

    if (lpszPWHistory[0] != _T(' ')) {
      rc = PWH_INVALID_CHARACTER;
      goto exit;
    }

    lpszPWHistory++;
    pwleft--;
    {
      StringX s3(lpszPWHistory, 4);
      iStringXStream is3(s3);
      is3 >> std::hex >> ipwlen;
    }
    lpszPWHistory += 4;
    pwleft -= 4;

    if (lpszPWHistory[0] != _T(' ')) {
      rc = PWH_INVALID_CHARACTER;
      goto exit;
    }

    lpszPWHistory += 1;
    pwleft -= 1;

    if (pwleft < ipwlen) {
      rc = PWH_INVALID_PSWD_LENGTH;
      goto exit;
    }

    tmp = StringX(lpszPWHistory, ipwlen);
    Format(buffer, _T("%08x%04x%s"), (long) t, ipwlen, tmp.c_str());
    newPWHistory += buffer.c_str();
    buffer.clear();
    lpszPWHistory += ipwlen;
    pwleft -= ipwlen;
  }

  if (pwleft > 0)
    rc = PWH_TOO_LONG;

 exit:
  Format(buffer, IDSC_PWHERROR, len - pwleft + 1);
  stringT temp;
  switch (rc) {
    case PWH_OK:
    case PWH_IGNORE:
      temp.clear();
      buffer.clear();
      break;
    case PWH_INVALID_HDR:
      Format(temp, IDSC_INVALIDHEADER, PWHistory.c_str());
      break;
    case PWH_INVALID_STATUS:
      Format(temp, IDSC_INVALIDPWHSTATUS, s);
      break;
    case PWH_INVALID_NUM:
      Format(temp, IDSC_INVALIDNUMOLDPW, n, m);
      break;
    case PWH_INVALID_DATETIME:
      LoadAString(temp, IDSC_INVALIDDATETIME);
      break;
    case PWH_INVALID_PSWD_LENGTH:
      LoadAString(temp, IDSC_INVALIDPWLENGTH);
      break;
    case PWH_TOO_SHORT:
      LoadAString(temp, IDSC_FIELDTOOSHORT);
      break;
    case PWH_TOO_LONG:
      LoadAString(temp, IDSC_FIELDTOOLONG);
      break;
    case PWH_INVALID_CHARACTER:
      LoadAString(temp, IDSC_INVALIDSEPARATER);
      break;
    default:
      ASSERT(0);
  }
  strErrors = buffer + temp;
  if (rc != PWH_OK)
    newPWHistory = _T("");

  return rc;
}

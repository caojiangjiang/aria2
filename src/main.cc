/* <!-- copyright */
/*
 * aria2 - The high speed download utility
 *
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */
#include "common.h"
#include "LogFactory.h"
#include "Util.h"
#include "prefs.h"
#include "FeatureConfig.h"
#include "MultiUrlRequestInfo.h"
#include "TorrentRequestInfo.h"
#include "BitfieldManFactory.h"
#include "SimpleRandomizer.h"
#include "ConsoleFileAllocationMonitor.h"
#include "Netrc.h"
#include "RequestFactory.h"
#include "OptionParser.h"
#include "OptionHandlerFactory.h"
#include "FatalException.h"
#include "File.h"
#include "CUIDCounter.h"
#include "UriFileListParser.h"
#include "CookieBoxFactory.h"
#include "a2algo.h"
#include <deque>
#include <algorithm>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <libgen.h>
#include <utility>
#include <fstream>
#include <sstream>
extern char* optarg;
extern int optind, opterr, optopt;
#include <getopt.h>

#ifdef ENABLE_METALINK
#include "MetalinkRequestInfo.h"
#include "Xml2MetalinkProcessor.h"
#endif

#ifdef HAVE_LIBSSL
// for SSL
# include <openssl/err.h>
# include <openssl/ssl.h>
#endif // HAVE_LIBSSL
#ifdef HAVE_LIBGNUTLS
# include <gnutls/gnutls.h>
#endif // HAVE_LIBGNUTLS

using namespace std;

void showVersion() {
  cout << PACKAGE << _(" version ") << PACKAGE_VERSION << endl;
  cout << "**Configuration**" << endl;
  cout << FeatureConfig::getInstance()->getConfigurationSummary();
  cout << endl;
  cout << "Copyright (C) 2006, 2007 Tatsuhiro Tsujikawa" << endl;
  cout << endl;
  cout <<
    _("This program is free software; you can redistribute it and/or modify\n"
      "it under the terms of the GNU General Public License as published by\n"
      "the Free Software Foundation; either version 2 of the License, or\n"
      "(at your option) any later version.\n"
      "\n"
      "This program is distributed in the hope that it will be useful,\n"
      "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "GNU General Public License for more details.\n"
      "\n"
      "You should have received a copy of the GNU General Public License\n"
      "along with this program; if not, write to the Free Software\n"
      "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA\n");
  cout << endl;
  printf(_("Contact Info: %s\n"), "Tatsuhiro Tsujikawa <tujikawa at users dot sourceforge dot net>");
  cout << endl;

}

void showUsage() {
  printf(_("Usage: %s [options] URL ...\n"), PACKAGE_NAME);
#ifdef ENABLE_BITTORRENT
  printf(_("       %s [options] -T TORRENT_FILE FILE ...\n"), PACKAGE_NAME);
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
  printf(_("       %s [options] -M METALINK_FILE\n"), PACKAGE_NAME);
#endif // ENABLE_METALINK
  cout << endl;
  cout << _("Options:") << endl;
  cout << _(" -d, --dir=DIR                The directory to store downloaded file.") << endl;
  cout << _(" -o, --out=FILE               The file name for downloaded file.") << endl;
  cout << _(" -l, --log=LOG                The file path to store log. If '-' is specified,\n"
	    "                              log is written to stdout.") << endl;
  cout << _(" -D, --daemon                 Run as daemon.") << endl;
  cout << _(" -s, --split=N                Download a file using N connections. N must be\n"
	    "                              between 1 and 5. This option affects all URLs.\n"
	    "                              Thus, aria2 connects to each URL with\n"
	    "                              N connections.\n"
	    "                              Default: 1") << endl;
  cout << _(" --retry-wait=SEC             Set amount of time in second between requests\n"
	    "                              for errors. Specify a value between 0 and 60.\n"
	    "                              Default: 5") << endl;
  cout << _(" -t, --timeout=SEC            Set timeout in second. Default: 60") << endl;
  cout << _(" -m, --max-tries=N            Set number of tries. 0 means unlimited.\n"
	    "                              Default: 5") << endl;
  /*
  cout << _(" --min-segment-size=SIZE[K|M] Set minimum segment size. You can append\n"
	    "                              K or M(1K = 1024, 1M = 1024K). This\n"
	    "                              value must be greater than or equal to\n"
	    "                              1024. Default: 1M") << endl;
  */
  cout << _(" --http-proxy=HOST:PORT       Use HTTP proxy server. This affects to all\n"
	    "                              URLs.") << endl;
  cout << _(" --http-user=USER             Set HTTP user. This affects to all URLs.") << endl;
  cout << _(" --http-passwd=PASSWD         Set HTTP password. This affects to all URLs.") << endl;
  cout << _(" --http-proxy-user=USER       Set HTTP proxy user. This affects to all URLs") << endl;
  cout << _(" --http-proxy-passwd=PASSWD   Set HTTP proxy password. This affects to all URLs.") << endl;
  cout << _(" --http-proxy-method=METHOD   Set the method to use in proxy request.\n"
	    "                              METHOD is either 'get' or 'tunnel'.\n"
	    "                              Default: tunnel") << endl;
  cout << _(" --http-auth-scheme=SCHEME    Set HTTP authentication scheme. Currently, basic\n"
	    "                              is the only supported scheme.\n"
	    "                              Default: basic") << endl;
  cout << _(" --referer=REFERER            Set Referer. This affects to all URLs.") << endl;
  cout << _(" --ftp-user=USER              Set FTP user. This affects to all URLs.\n"
	    "                              Default: anonymous") << endl;
  cout << _(" --ftp-passwd=PASSWD          Set FTP password. This affects to all URLs.\n"
	    "                              Default: ARIA2USER@") << endl;
  cout << _(" --ftp-type=TYPE              Set FTP transfer type. TYPE is either 'binary'\n"
	    "                              or 'ascii'.\n"
	    "                              Default: binary") << endl;
  cout << _(" -p, --ftp-pasv               Use passive mode in FTP.") << endl;
  cout << _(" --ftp-via-http-proxy=METHOD  Use HTTP proxy in FTP. METHOD is either 'get' or\n"
	    "                              'tunnel'.\n"
	    "                              Default: tunnel") << endl;
  cout << _(" --lowest-speed-limit=SPEED   Close connection if download speed is lower than\n"
	    "                              or equal to this value(bytes per sec).\n"
	    "                              0 means aria2 does not care lowest speed limit.\n"
	    "                              You can append K or M(1K = 1024, 1M = 1024K).\n"

	    "                              This option does not affect BitTorrent download.\n"
	    "                              Default: 0") << endl;
  cout << _(" --max-download-limit=SPEED   Set max download speed in bytes per sec.\n"
	    "                              0 means unrestricted.\n"
	    "                              You can append K or M(1K = 1024, 1M = 1024K).\n"
	    "                              Default: 0") << endl;
  cout << _(" --file-allocation=METHOD     Specify file allocation method. METHOD is either\n"
	    "                              'none' or 'prealloc'.\n"
	    "                              'none' doesn't pre-allocate file space. 'prealloc'\n"
	    "                              pre-allocates file space before download begins.\n"
	    "                              This may take some time depending on the size of\n"
	    "                              file.\n"
	    "                              Default: none") << endl;
  cout << _(" --allow-overwrite=true|false  If this option set to false, aria2 doesn't\n"
	    "                              download a file which already exists in the file\n"
	    "                              system but its corresponding .aria2 file doesn't\n"
	    "                              exist.\n"
            "                              Default: false") << endl;
#ifdef ENABLE_MESSAGE_DIGEST
  cout << _(" --check-integrity=true|false  Check file integrity by validating piece hash.\n"
	    "                              This option makes effect in BitTorrent download\n"
	    "                              and Metalink with chunk checksums.\n"
	    "                              Use this option to redownload a damaged portion of\n"
	    "                              file.\n"
	    "                              You may need to specify --allow-overwrite=true\n"
	    "                              option if .aria2 file doesn't exist.\n"
	    "                              Default: false") << endl;
  cout << _(" --realtime-chunk-checksum=true|false  Validate chunk checksum while downloading\n"
	    "                              a file in Metalink mode. This option makes effect\n"
	    "                              in Metalink with chunk checksums.\n"
	    "                              Default: true") << endl;
#endif // ENABLE_MESSAGE_DIGEST
  cout << _(" -c, --continue               Continue downloading a partially downloaded\n"
	    "                              file. Use this option to resume a download started\n"
	    "                              by web browsers or another programs\n"
	    "                              which download files sequentially from the\n"
	    "                              beginning. Currently this option is applicable to\n"
	    "                              http(s)/ftp downloads.") << endl;
  cout << _(" -U, --user-agent=USER_AGENT  Set user agent for http(s) downloads.") << endl;
  cout << _(" -n, --no-netrc               Disables netrc support.") << endl;
  cout << _(" -i, --input-file=FILE        Downloads URIs found in FILE. You can specify\n"
	    "                              multiple URIs for a single entity: deliminate\n"
	    "                              URIs by Tab in a single line.") << endl;
  cout << _(" -j, --max-concurrent-downloads=N Set maximum number of concurrent downloads.\n"
	    "                              It should be used with -i option.\n"
	    "                              Default: 5") << endl;
  cout << _(" --load-cookies=FILE          Load cookies from FILE. The format of FILE is\n"
	    "                              one used by Netscape and Mozilla.") << endl;
#if defined ENABLE_BITTORRENT || ENABLE_METALINK
  cout << _(" -S, --show-files             Print file listing of .torrent or .metalink file\n"
	    "                              and exit.") << endl;
  cout << _(" --select-file=INDEX...       Set file to download by specifing its index.\n"
	    "                              You can know file index through --show-files\n"
	    "                              option. Multiple indexes can be specified by using\n"
	    "                              ',' like \"3,6\".\n"
	    "                              You can also use '-' to specify rangelike \"1-5\".\n"
	    "                              ',' and '-' can be used together.\n"
	    "                              When used with -M option, index may vary depending\n"
	    "                              on the query(see --metalink-* options).") << endl;
#endif // ENABLE_BITTORRENT || ENABLE_METALINK
#ifdef ENABLE_BITTORRENT
  cout << _(" -T, --torrent-file=TORRENT_FILE  The file path to .torrent file.") << endl;
  cout << _(" --follow-torrent=true|false  Setting this option to false prevents aria2 to\n"
	    "                              enter BitTorrent mode even if the filename of\n"
	    "                              downloaded file ends with .torrent.\n"
	    "                              Default: true") << endl;
  cout << _(" --direct-file-mapping=true|false Directly read from and write to each file\n"
	    "                              mentioned in .torrent file.\n"
	    "                              Default: true") << endl;
  cout << _(" --listen-port=PORT           Set port number to listen to for peer connection.\n"
	    "                              Default: 6881-6999") << endl;
  cout << _(" --max-upload-limit=SPEED     Set max upload speed in bytes per sec.\n"
	    "                              0 means unrestricted.\n"
	    "                              You can append K or M(1K = 1024, 1M = 1024K).\n"
	    "                              Default: 0") << endl;
  cout << _(" --seed-time=MINUTES          Specify seeding time in minutes. See also\n"
	    "                              --seed-ratio option.") << endl;
  cout << _(" --seed-ratio=RATIO           Specify share ratio. Seed completed torrents until\n"
	    "                              share ratio reaches RATIO. 1.0 is encouraged.\n"
	    "                              If --seed-time option is specified along with\n"
	    "                              this option, seeding ends when at least one of\n"
	    "                              the conditions is satisfied.") << endl;
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
  cout << _(" -M, --metalink-file=METALINK_FILE The file path to .metalink file.") << endl;
  cout << _(" -C, --metalink-servers=NUM_SERVERS The number of servers to connect to\n"
	    "                              simultaneously.\n"
	    "                              Default: 5") << endl;
  cout << _(" --metalink-version=VERSION   The version of file to download.") << endl;
  cout << _(" --metalink-language=LANGUAGE The language of file to download.") << endl;
  cout << _(" --metalink-os=OS             The operating system the file is targeted.") << endl;
  cout << _(" --metalink-location=LOCATION The location of the prefered server.") << endl;
  cout << _(" --follow-metalink=true|false  Setting this option to false prevents aria2 to\n"
	    "                              enter Metalink mode even if the filename of\n"
	    "                              downloaded file ends with .metalink.\n"
	    "                              Default: true") << endl;
#endif // ENABLE_METALINK
  cout << _(" -v, --version                Print the version number and exit.") << endl;
  cout << _(" -h, --help                   Print this message and exit.") << endl;
  cout << endl;
  cout << "URL:" << endl;
  cout << _(" You can specify multiple URLs. All URLs must point to the same file\n"
	    " or downloading fails.") << endl;
  cout << endl;
#ifdef ENABLE_BITTORRENT
  cout << "FILE:" << endl;
  cout << _(" Specify files in multi-file torrent to download. Use conjunction with\n"
	    " -T option. This arguments are ignored if you specify --select-file option.") << endl;
  cout << endl;
#endif // ENABLE_BITTORRENT
  cout << _("Examples:") << endl;
  cout << _(" Download a file by 1 connection:") << endl;
  cout << "  aria2c http://AAA.BBB.CCC/file.zip" << endl;
  cout << _(" Download a file by 2 connections:") << endl;
  cout << "  aria2c -s 2 http://AAA.BBB.CCC/file.zip" << endl;
  cout << _(" Download a file by 2 connections, each connects to a different server:") << endl;
  cout << "  aria2c http://AAA.BBB.CCC/file.zip http://DDD.EEE.FFF/GGG/file.zip" << endl;
  cout << _(" You can mix up different protocols:") << endl;
  cout << "  aria2c http://AAA.BBB.CCC/file.zip ftp://DDD.EEE.FFF/GGG/file.zip" << endl;
#ifdef ENABLE_BITTORRENT
  cout << endl;
  cout << _(" Download a torrent:") << endl;
  cout << "  aria2c -o test.torrent http://AAA.BBB.CCC/file.torrent" << endl;
  cout << _(" Download a torrent using local .torrent file:") << endl;
  cout << "  aria2c -T test.torrent" << endl;
  cout << _(" Download only selected files:") << endl;
  cout << "  aria2c -T test.torrent dir/file1.zip dir/file2.zip" << endl;
  cout << _(" Print file listing of .torrent file:") << endl;
  cout << "  aria2c -T test.torrent -S" << endl;  
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
  cout << endl;
  cout << _(" Metalink downloading:") << endl;
  cout << "  aria2c http://AAA.BBB.CCC/file.metalink" << endl;
  cout << _(" Download a file using local .metalink file:") << endl;
  cout << "  aria2c -M test.metalink" << endl;
  cout << _(" Metalink downloading with preferences:") << endl;
  cout << "  aria2c -M test.metalink --metalink-version=1.1.1 --metalink-language=en-US" << endl;
#endif // ENABLE_METALINK
  cout << endl;
  printf(_("Report bugs to %s"), "<tujikawa at users dot sourceforge dot net>");
  cout << endl;
}

int main(int argc, char* argv[]) {
#ifdef ENABLE_NLS
  setlocale (LC_CTYPE, "");
  setlocale (LC_MESSAGES, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
#endif // ENABLE_NLS
  stringstream cmdstream;
  int c;
  Option* op = new Option();
  op->put(PREF_STDOUT_LOG, V_FALSE);
  op->put(PREF_DIR, ".");
  op->put(PREF_SPLIT, "1");
  op->put(PREF_DAEMON, V_FALSE);
  op->put(PREF_SEGMENT_SIZE, Util::itos(1024*1024));
  op->put(PREF_HTTP_KEEP_ALIVE, V_FALSE);
  op->put(PREF_LISTEN_PORT, "-1");
  op->put(PREF_METALINK_SERVERS, "5");
  op->put(PREF_FOLLOW_TORRENT,
#ifdef ENABLE_BITTORRENT
	  V_TRUE
#else
	  V_FALSE
#endif // ENABLE_BITTORRENT
	  );
  op->put(PREF_FOLLOW_METALINK,
#ifdef ENABLE_METALINK
	  V_TRUE
#else
	  V_FALSE
#endif // ENABLE_METALINK
	  );
  op->put(PREF_RETRY_WAIT, "5");
  op->put(PREF_TIMEOUT, "60");
  op->put(PREF_DNS_TIMEOUT, "10");
  op->put(PREF_PEER_CONNECTION_TIMEOUT, "60");
  op->put(PREF_BT_TIMEOUT, "180");
  op->put(PREF_BT_REQUEST_TIMEOUT, "60");
  op->put(PREF_BT_KEEP_ALIVE_INTERVAL, "120");
  op->put(PREF_MIN_SEGMENT_SIZE, "1048576");// 1M
  op->put(PREF_MAX_TRIES, "5");
  op->put(PREF_HTTP_AUTH_SCHEME, V_BASIC);
  op->put(PREF_HTTP_PROXY_METHOD, V_TUNNEL);
  op->put(PREF_FTP_TYPE, V_BINARY);
  op->put(PREF_FTP_VIA_HTTP_PROXY, V_TUNNEL);
  op->put(PREF_AUTO_SAVE_INTERVAL, "60");
  op->put(PREF_DIRECT_FILE_MAPPING, V_TRUE);
  op->put(PREF_LOWEST_SPEED_LIMIT, "0");
  op->put(PREF_MAX_DOWNLOAD_LIMIT, "0");
  op->put(PREF_MAX_UPLOAD_LIMIT, "0");
  op->put(PREF_STARTUP_IDLE_TIME, "10");
  op->put(PREF_TRACKER_MAX_TRIES, "10");
  op->put(PREF_FILE_ALLOCATION, V_NONE);
  op->put(PREF_ALLOW_OVERWRITE, V_FALSE);
  op->put(PREF_REALTIME_CHUNK_CHECKSUM, V_TRUE);
  op->put(PREF_CHECK_INTEGRITY, V_FALSE);
  op->put(PREF_NETRC_PATH, Util::getHomeDir()+"/.netrc");
  op->put(PREF_CONTINUE, V_FALSE);
  op->put(PREF_USER_AGENT, "aria2");
  op->put(PREF_NO_NETRC, V_FALSE);
  op->put(PREF_MAX_CONCURRENT_DOWNLOADS, "5");
  op->put(PREF_DIRECT_DOWNLOAD_TIMEOUT, "15");
  while(1) {
    int optIndex = 0;
    int lopt;
    static struct option longOpts[] = {
      { "daemon", no_argument, NULL, 'D' },
      { "dir", required_argument, NULL, 'd' },
      { "out", required_argument, NULL, 'o' },
      { "log", required_argument, NULL, 'l' },
      { "split", required_argument, NULL, 's' },
      { "timeout", required_argument, NULL, 't' },
      { "max-tries", required_argument, NULL, 'm' },
      { "http-proxy", required_argument, &lopt, 1 },
      { "http-user", required_argument, &lopt, 2 },
      { "http-passwd", required_argument, &lopt, 3 },
      { "http-proxy-user", required_argument, &lopt, 4 },
      { "http-proxy-passwd", required_argument, &lopt, 5 },
      { "http-auth-scheme", required_argument, &lopt, 6 },
      { "referer", required_argument, &lopt, 7 },
      { "retry-wait", required_argument, &lopt, 8 },
      { "ftp-user", required_argument, &lopt, 9 },
      { "ftp-passwd", required_argument, &lopt, 10 },
      { "ftp-type", required_argument, &lopt, 11 },
      { "ftp-pasv", no_argument, NULL, 'p' },
      { "ftp-via-http-proxy", required_argument, &lopt, 12 },
      //{ "min-segment-size", required_argument, &lopt, 13 },
      { "http-proxy-method", required_argument, &lopt, 14 },
      { "lowest-speed-limit", required_argument, &lopt, 200 },
      { "max-download-limit", required_argument, &lopt, 201 },
      { "file-allocation", required_argument, 0, 'a' },
      { "allow-overwrite", required_argument, &lopt, 202 },
#ifdef ENABLE_MESSAGE_DIGEST
      { "check-integrity", required_argument, &lopt, 203 },
      { "realtime-chunk-checksum", required_argument, &lopt, 204 },
#endif // ENABLE_MESSAGE_DIGEST
      { "continue", no_argument, 0, 'c' },
      { "user-agent", required_argument, 0, 'U' },
      { "no-netrc", no_argument, 0, 'n' },
      { "input-file", required_argument, 0, 'i' },
      { "max-concurrent-downloads", required_argument, 0, 'j' },
      { "load-cookies", required_argument, &lopt, 205 },
#if defined ENABLE_BITTORRENT || ENABLE_METALINK
      { "show-files", no_argument, NULL, 'S' },
      { "select-file", required_argument, &lopt, 21 },
#endif // ENABLE_BITTORRENT || ENABLE_METALINK
#ifdef ENABLE_BITTORRENT
      { "torrent-file", required_argument, NULL, 'T' },
      { "listen-port", required_argument, &lopt, 15 },
      { "follow-torrent", required_argument, &lopt, 16 },
      { "no-preallocation", no_argument, &lopt, 18 },
      { "direct-file-mapping", required_argument, &lopt, 19 },
      // TODO remove upload-limit.
      //{ "upload-limit", required_argument, &lopt, 20 },
      { "seed-time", required_argument, &lopt, 22 },
      { "seed-ratio", required_argument, &lopt, 23 },
      { "max-upload-limit", required_argument, &lopt, 24 },
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
      { "metalink-file", required_argument, NULL, 'M' },
      { "metalink-servers", required_argument, NULL, 'C' },
      { "metalink-version", required_argument, &lopt, 100 },
      { "metalink-language", required_argument, &lopt, 101 },
      { "metalink-os", required_argument, &lopt, 102 },
      { "follow-metalink", required_argument, &lopt, 103 },
      { "metalink-location", required_argument, &lopt, 104 },
#endif // ENABLE_METALINK
      { "version", no_argument, NULL, 'v' },
      { "help", no_argument, NULL, 'h' },
      { 0, 0, 0, 0 }
    };
    c = getopt_long(argc, argv, "Dd:o:l:s:pt:m:vhST:M:C:a:cU:ni:j:", longOpts, &optIndex);
    if(c == -1) {
      break;
    }
    switch(c) {
    case 0:{
      switch(lopt) {
      case 1:
	cmdstream << PREF_HTTP_PROXY << "=" << optarg << "\n";
	break;
      case 2:
	cmdstream << PREF_HTTP_USER << "=" << optarg << "\n";
	break;
      case 3:
	cmdstream << PREF_HTTP_PASSWD << "=" << optarg << "\n";
	break;
      case 4:
	cmdstream << PREF_HTTP_PROXY_USER << "=" << optarg << "\n";
	break;
      case 5: 
	cmdstream << PREF_HTTP_PROXY_PASSWD << "=" << optarg << "\n";
	break;
      case 6:
	cmdstream << PREF_HTTP_AUTH_SCHEME << "=" << optarg << "\n";
	break;
      case 7:
	cmdstream << PREF_REFERER << "=" << optarg << "\n";
	break;
      case 8:
	cmdstream << PREF_RETRY_WAIT << "=" << optarg << "\n";
	break;
      case 9:
	cmdstream << PREF_FTP_USER << "=" << optarg << "\n";
	break;
      case 10:
	cmdstream << PREF_FTP_PASSWD << "=" << optarg << "\n";
	break;
      case 11:
	cmdstream << PREF_FTP_TYPE << "=" << optarg << "\n";
	break;
      case 12:
	cmdstream << PREF_FTP_VIA_HTTP_PROXY << "=" << optarg << "\n";
	break;
      case 13:
	cmdstream << PREF_MIN_SEGMENT_SIZE << "=" << optarg << "\n";
	break;
      case 14:
	cmdstream << PREF_HTTP_PROXY_METHOD << "=" << optarg << "\n";
	break;
      case 15:
	cmdstream << PREF_LISTEN_PORT << "=" << optarg << "\n";
	break;
      case 16:
	cmdstream << PREF_FOLLOW_TORRENT << "=" << optarg << "\n";
	break;
      case 18:
	cmdstream << PREF_NO_PREALLOCATION << "=" << V_TRUE << "\n";
	break;
      case 19:
	cmdstream << PREF_DIRECT_FILE_MAPPING << "=" << optarg << "\n";
	break;
      case 21:
	cmdstream << PREF_SELECT_FILE << "=" << optarg << "\n";
	break;
      case 22:
	cmdstream << PREF_SEED_TIME << "=" << optarg << "\n";
	break;
      case 23:
	cmdstream << PREF_SEED_RATIO << "=" << optarg << "\n";
	break;
      case 24:
	cmdstream << PREF_MAX_UPLOAD_LIMIT << "=" << optarg << "\n";
	break;
      case 100:
	cmdstream << PREF_METALINK_VERSION << "=" << optarg << "\n";
	break;
      case 101:
	cmdstream << PREF_METALINK_LANGUAGE << "=" << optarg << "\n";
	break;
      case 102:
	cmdstream << PREF_METALINK_OS << "=" << optarg << "\n";
	break;
      case 103:
	cmdstream << PREF_FOLLOW_METALINK << "=" << optarg << "\n";
	break;
      case 104:
	cmdstream << PREF_METALINK_LOCATION << "=" << optarg << "\n";
	break;
      case 200:
	cmdstream << PREF_LOWEST_SPEED_LIMIT << "=" << optarg << "\n";
	break;
      case 201:
	cmdstream << PREF_MAX_DOWNLOAD_LIMIT << "=" << optarg << "\n";
	break;
      case 202:
	cmdstream << PREF_ALLOW_OVERWRITE << "=" << optarg << "\n";
	break;
      case 203:
	cmdstream << PREF_CHECK_INTEGRITY << "=" << optarg << "\n";
	break;
      case 204:
	cmdstream << PREF_REALTIME_CHUNK_CHECKSUM << "=" << optarg << "\n";
	break;
      case 205:
	cmdstream << PREF_LOAD_COOKIES << "=" << optarg << "\n";
	break;
      }
      break;
    }
    case 'D':
      cmdstream << PREF_DAEMON << "=" << V_TRUE << "\n";
      break;
    case 'd':
      cmdstream << PREF_DIR << "=" << optarg << "\n";
      break;
    case 'o':
      cmdstream << PREF_OUT << "=" << optarg << "\n";
      break;
    case 'l':
      cmdstream << PREF_LOG << "=" << optarg << "\n";
      break;
    case 's':
      cmdstream << PREF_SPLIT << "=" << optarg << "\n";
      break;
    case 't':
      cmdstream << PREF_TIMEOUT << "=" << optarg << "\n";
      break;
    case 'm':
      cmdstream << PREF_MAX_TRIES << "=" << optarg << "\n";
      break;
    case 'p':
      cmdstream << PREF_FTP_PASV << "=" << V_TRUE << "\n";
      break;
    case 'S':
      cmdstream << PREF_SHOW_FILES << "=" << V_TRUE << "\n";
      break;
    case 'T':
      cmdstream << PREF_TORRENT_FILE << "=" << optarg << "\n";
      break;
    case 'M':
      cmdstream << PREF_METALINK_FILE << "=" << optarg << "\n";
      break;
    case 'C':
      cmdstream << PREF_METALINK_SERVERS << "=" << optarg << "\n";
      break;
    case 'a':
      cmdstream << PREF_FILE_ALLOCATION << "=" << optarg << "\n";
      break;
    case 'c':
      cmdstream << PREF_CONTINUE << "=" << V_TRUE << "\n";
      break;
    case 'U':
      cmdstream << PREF_USER_AGENT << "=" << optarg << "\n";
      break;
    case 'n':
      cmdstream << PREF_NO_NETRC << "=" << V_TRUE << "\n";
      break;
    case 'i':
      cmdstream << PREF_INPUT_FILE << "=" << optarg << "\n";
      break;
    case 'j':
      cmdstream << PREF_MAX_CONCURRENT_DOWNLOADS << "=" << optarg << "\n";
      break;
    case 'v':
      showVersion();
      exit(EXIT_SUCCESS);
    case 'h':
      showUsage();
      exit(EXIT_SUCCESS);
    default:
      exit(EXIT_FAILURE);
    }
  }

  {
    OptionParser oparser;
    oparser.setOptionHandlers(OptionHandlerFactory::createOptionHandlers());
    string cfname = Util::getHomeDir()+"/.aria2/aria2.conf";
    ifstream cfstream(cfname.c_str());
    try {
      oparser.parse(op, cfstream);
    } catch(Exception* e) {
      cerr << "Parse error in " << cfname << endl;
      cerr << e->getMsg() << endl;
      delete e;
      exit(EXIT_FAILURE);
    }
    try {
      oparser.parse(op, cmdstream);
    } catch(Exception* e) {
      cerr << e->getMsg() << endl;
      delete e;
      exit(EXIT_FAILURE);
    }
  }
  if(op->defined(PREF_HTTP_USER)) {
    op->put(PREF_HTTP_AUTH_ENABLED, V_TRUE);
  }
  if(op->defined(PREF_HTTP_PROXY_USER)) {
    op->put(PREF_HTTP_PROXY_AUTH_ENABLED, V_TRUE);
  }
  if(
#ifdef ENABLE_BITTORRENT
     !op->defined(PREF_TORRENT_FILE) &&
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
     !op->defined(PREF_METALINK_FILE) &&
#endif // ENABLE_METALINK
     !op->defined(PREF_INPUT_FILE)) {
    if(optind == argc) {
      cerr << _("specify at least one URL") << endl;
      exit(EXIT_FAILURE);
    }
  }
  if(op->getAsBool(PREF_DAEMON)) {
    if(daemon(1, 1) < 0) {
      perror(_("daemon failed"));
      exit(EXIT_FAILURE);
    }
  }
  Strings args(argv+optind, argv+argc);
  
#ifdef HAVE_LIBSSL
  // for SSL initialization
  SSL_load_error_strings();
  SSL_library_init();
#endif // HAVE_LIBSSL
#ifdef HAVE_LIBGNUTLS
  gnutls_global_init();
#endif // HAVE_LIBGNUTLS
#ifdef ENABLE_METALINK
  xmlInitParser();
#endif // ENABLE_METALINK
  SimpleRandomizer::init();
  BitfieldManFactory::setDefaultRandomizer(SimpleRandomizer::getInstance());
  FileAllocationMonitorFactory::setFactory(new ConsoleFileAllocationMonitorFactory());
  if(op->getAsBool(PREF_STDOUT_LOG)) {
    LogFactory::setLogFile("/dev/stdout");
  } else if(op->get(PREF_LOG).size()) {
    LogFactory::setLogFile(op->get(PREF_LOG));
  } else {
    LogFactory::setLogFile("/dev/null");
  }
  int exitStatus = EXIT_SUCCESS;
  try {
    Logger* logger = LogFactory::getInstance();
    logger->info("%s %s", PACKAGE, PACKAGE_VERSION);
    logger->info("Logging started.");

    RequestFactoryHandle requestFactory = new RequestFactory();
    requestFactory->setOption(op);
    File netrccf(op->get(PREF_NETRC_PATH));
    if(!op->getAsBool(PREF_NO_NETRC) && netrccf.isFile()) {
      mode_t mode = netrccf.mode();
      if(mode&(S_IRWXG|S_IRWXO)) {
	logger->notice(".netrc file %s does not have correct permissions. It should be 600. netrc support disabled.",
		       op->get(PREF_NETRC_PATH).c_str());
      } else {
	NetrcHandle netrc = new Netrc();
	netrc->parse(op->get(PREF_NETRC_PATH));
	requestFactory->setNetrc(netrc);
      }
    }

    CookieBoxFactoryHandle cookieBoxFactory = new CookieBoxFactory();
    CookieBoxFactorySingletonHolder::instance(cookieBoxFactory);
    if(op->defined(PREF_LOAD_COOKIES)) {
      File cookieFile(op->get(PREF_LOAD_COOKIES));
      if(cookieFile.isFile()) {
	ifstream in(op->get(PREF_LOAD_COOKIES).c_str());
	CookieBoxFactorySingletonHolder::instance()->loadDefaultCookie(in);
      } else {
	logger->error("Failed to load cookies from %s", op->get(PREF_LOAD_COOKIES).c_str());
	exit(EXIT_FAILURE);
      }
    }

    RequestFactorySingletonHolder::instance(requestFactory);
    CUIDCounterHandle cuidCounter = new CUIDCounter();
    CUIDCounterSingletonHolder::instance(cuidCounter);

    Util::setGlobalSignalHandler(SIGPIPE, SIG_IGN, 0);

    RequestInfo* firstReqInfo;
#ifdef ENABLE_BITTORRENT
    if(op->defined(PREF_TORRENT_FILE)) {
      firstReqInfo = new TorrentRequestInfo(op->get(PREF_TORRENT_FILE),
					   op);
      Strings targetFiles;
      if(op->defined(PREF_TORRENT_FILE) && !args.empty()) {
	targetFiles = args;
      }
      ((TorrentRequestInfo*)firstReqInfo)->setTargetFiles(targetFiles);
    }
    else
#endif // ENABLE_BITTORRENT
#ifdef ENABLE_METALINK
      if(op->defined(PREF_METALINK_FILE)) {
	firstReqInfo = new MetalinkRequestInfo(op->get(PREF_METALINK_FILE),
					       op);
	Strings targetFiles;
	if(op->defined(PREF_METALINK_FILE) && !args.empty()) {
	  targetFiles = args;
	}
	((MetalinkRequestInfo*)firstReqInfo)->setTargetFiles(targetFiles);
      }
      else
#endif // ENABLE_METALINK
	if(op->defined(PREF_INPUT_FILE)) {
	  UriFileListParser flparser(op->get(PREF_INPUT_FILE));
	  RequestGroups groups;
	  while(flparser.hasNext()) {
	    Strings uris = flparser.next();
	    if(!uris.empty()) {
	      Strings xuris;
	      ncopy(uris.begin(), uris.end(), op->getAsInt(PREF_SPLIT),
		    back_inserter(xuris));
	      RequestGroupHandle rg = new RequestGroup(xuris, op);
	      groups.push_back(rg);
	    }
	  }
	  firstReqInfo = new MultiUrlRequestInfo(groups, op);
	}
	else
	  {
	    Strings xargs;
	    ncopy(args.begin(), args.end(), op->getAsInt(PREF_SPLIT),
		  back_inserter(xargs));
	    firstReqInfo = new MultiUrlRequestInfo(xargs, op);
	  }

    RequestInfos reqInfos;
    if(firstReqInfo) {
      reqInfos.push_front(firstReqInfo);
    }
    while(reqInfos.size()) {
      RequestInfoHandle reqInfo = reqInfos.front();
      reqInfos.pop_front();
      RequestInfos nextReqInfos = reqInfo->execute();
      copy(nextReqInfos.begin(), nextReqInfos.end(), front_inserter(reqInfos));
      /*
      if(reqInfo->isFail()) {
	exitStatus = EXIT_FAILURE;
      } else if(reqInfo->getFileInfo().checkReady()) {
	cout << _("Now verifying checksum.\n"
		  "This may take some time depending on your PC environment"
		  " and the size of file.") << endl;
	if(reqInfo->getFileInfo().check()) {
	  cout << _("checksum OK.") << endl;
	} else {
	  // TODO
	  cout << _("checksum ERROR.") << endl;
	  exitStatus = EXIT_FAILURE;
	}
      }
      */
    }
  } catch(Exception* ex) {
    cerr << "Exception caught:\n" << ex->getMsg() << endl;
    delete ex;
    exit(EXIT_FAILURE);
  }
  delete op;
  LogFactory::release();
#ifdef HAVE_LIBGNUTLS
  gnutls_global_deinit();
#endif // HAVE_LIBGNUTLS
#ifdef ENABLE_METALINK
  xmlCleanupParser();
#endif // ENABLE_METALINK
  FeatureConfig::release();
  return exitStatus;
}

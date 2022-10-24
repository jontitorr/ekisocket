#include <algorithm>
#include <array>
#include <ekisocket/Util.hpp>
#include <fmt/format.h>
#include <functional>
#include <openssl/sha.h>
#include <random>
#include <unordered_map>

namespace {
// Static Constexpr Map of all the file extension to their content type.
using namespace std::literals::string_view_literals;
const std::unordered_map<std::string_view, std::string_view> CONTENT_TYPES {
    { "*3gpp"sv, "audio/3gpp"sv },
    { "*jpm"sv, "video/jpm"sv },
    { "*mp3"sv, "audio/mp3"sv },
    { "*rtf"sv, "text/rtf"sv },
    { "*wav"sv, "audio/wave"sv },
    { "*xml"sv, "text/xml"sv },
    { "3g2"sv, "video/3gpp2"sv },
    { "3gp"sv, "video/3gpp"sv },
    { "3gpp"sv, "video/3gpp"sv },
    { "ac"sv, "application/pkix-attr-cert"sv },
    { "adp"sv, "audio/adpcm"sv },
    { "ai"sv, "application/postscript"sv },
    { "apng"sv, "image/apng"sv },
    { "appcache"sv, "text/cache-manifest"sv },
    { "asc"sv, "application/pgp-signature"sv },
    { "atom"sv, "application/atom+xml"sv },
    { "atomcat"sv, "application/atomcat+xml"sv },
    { "atomsvc"sv, "application/atomsvc+xml"sv },
    { "au"sv, "audio/basic"sv },
    { "aw"sv, "application/applixware"sv },
    { "bdoc"sv, "application/bdoc"sv },
    { "bin"sv, "application/octet-stream"sv },
    { "bmp"sv, "image/bmp"sv },
    { "bpk"sv, "application/octet-stream"sv },
    { "buffer"sv, "application/octet-stream"sv },
    { "ccxml"sv, "application/ccxml+xml"sv },
    { "cdmia"sv, "application/cdmi-capability"sv },
    { "cdmic"sv, "application/cdmi-container"sv },
    { "cdmid"sv, "application/cdmi-domain"sv },
    { "cdmio"sv, "application/cdmi-object"sv },
    { "cdmiq"sv, "application/cdmi-queue"sv },
    { "cer"sv, "application/pkix-cert"sv },
    { "cgm"sv, "image/cgm"sv },
    { "class"sv, "application/java-vm"sv },
    { "coffee"sv, "text/coffeescript"sv },
    { "conf"sv, "text/plain"sv },
    { "cpt"sv, "application/mac-compactpro"sv },
    { "crl"sv, "application/pkix-crl"sv },
    { "css"sv, "text/css"sv },
    { "csv"sv, "text/csv"sv },
    { "cu"sv, "application/cu-seeme"sv },
    { "davmount"sv, "application/davmount+xml"sv },
    { "dbk"sv, "application/docbook+xml"sv },
    { "deb"sv, "application/octet-stream"sv },
    { "def"sv, "text/plain"sv },
    { "deploy"sv, "application/octet-stream"sv },
    { "disposition-notification"sv, "message/disposition-notification"sv },
    { "dist"sv, "application/octet-stream"sv },
    { "distz"sv, "application/octet-stream"sv },
    { "dll"sv, "application/octet-stream"sv },
    { "dmg"sv, "application/octet-stream"sv },
    { "dms"sv, "application/octet-stream"sv },
    { "doc"sv, "application/msword"sv },
    { "dot"sv, "application/msword"sv },
    { "drle"sv, "image/dicom-rle"sv },
    { "dssc"sv, "application/dssc+der"sv },
    { "dtd"sv, "application/xml-dtd"sv },
    { "dump"sv, "application/octet-stream"sv },
    { "ear"sv, "application/java-archive"sv },
    { "ecma"sv, "application/ecmascript"sv },
    { "elc"sv, "application/octet-stream"sv },
    { "emf"sv, "image/emf"sv },
    { "eml"sv, "message/rfc822"sv },
    { "emma"sv, "application/emma+xml"sv },
    { "eps"sv, "application/postscript"sv },
    { "epub"sv, "application/epub+zip"sv },
    { "es"sv, "application/ecmascript"sv },
    { "exe"sv, "application/octet-stream"sv },
    { "exi"sv, "application/exi"sv },
    { "exr"sv, "image/aces"sv },
    { "ez"sv, "application/andrew-inset"sv },
    { "fits"sv, "image/fits"sv },
    { "g3"sv, "image/g3fax"sv },
    { "gbr"sv, "application/rpki-ghostbusters"sv },
    { "geojson"sv, "application/geo+json"sv },
    { "gif"sv, "image/gif"sv },
    { "glb"sv, "model/gltf-binary"sv },
    { "gltf"sv, "model/gltf+json"sv },
    { "gml"sv, "application/gml+xml"sv },
    { "gpx"sv, "application/gpx+xml"sv },
    { "gram"sv, "application/srgs"sv },
    { "grxml"sv, "application/srgs+xml"sv },
    { "gxf"sv, "application/gxf"sv },
    { "gz"sv, "application/gzip"sv },
    { "h261"sv, "video/h261"sv },
    { "h263"sv, "video/h263"sv },
    { "h264"sv, "video/h264"sv },
    { "heic"sv, "image/heic"sv },
    { "heics"sv, "image/heic-sequence"sv },
    { "heif"sv, "image/heif"sv },
    { "heifs"sv, "image/heif-sequence"sv },
    { "hjson"sv, "application/hjson"sv },
    { "hlp"sv, "application/winhlp"sv },
    { "hqx"sv, "application/mac-binhex40"sv },
    { "htm"sv, "text/html"sv },
    { "html"sv, "text/html"sv },
    { "ics"sv, "text/calendar"sv },
    { "ief"sv, "image/ief"sv },
    { "ifb"sv, "text/calendar"sv },
    { "iges"sv, "model/iges"sv },
    { "igs"sv, "model/iges"sv },
    { "img"sv, "application/octet-stream"sv },
    { "in"sv, "text/plain"sv },
    { "ini"sv, "text/plain"sv },
    { "ink"sv, "application/inkml+xml"sv },
    { "inkml"sv, "application/inkml+xml"sv },
    { "ipfix"sv, "application/ipfix"sv },
    { "iso"sv, "application/octet-stream"sv },
    { "jade"sv, "text/jade"sv },
    { "jar"sv, "application/java-archive"sv },
    { "jls"sv, "image/jls"sv },
    { "jp2"sv, "image/jp2"sv },
    { "jpe"sv, "image/jpeg"sv },
    { "jpeg"sv, "image/jpeg"sv },
    { "jpf"sv, "image/jpx"sv },
    { "jpg"sv, "image/jpeg"sv },
    { "jpg2"sv, "image/jp2"sv },
    { "jpgm"sv, "video/jpm"sv },
    { "jpgv"sv, "video/jpeg"sv },
    { "jpm"sv, "image/jpm"sv },
    { "jpx"sv, "image/jpx"sv },
    { "js"sv, "application/javascript"sv },
    { "json"sv, "application/json"sv },
    { "json5"sv, "application/json5"sv },
    { "jsonld"sv, "application/ld+json"sv },
    { "jsonml"sv, "application/jsonml+json"sv },
    { "jsx"sv, "text/jsx"sv },
    { "kar"sv, "audio/midi"sv },
    { "ktx"sv, "image/ktx"sv },
    { "less"sv, "text/less"sv },
    { "list"sv, "text/plain"sv },
    { "litcoffee"sv, "text/coffeescript"sv },
    { "log"sv, "text/plain"sv },
    { "lostxml"sv, "application/lost+xml"sv },
    { "lrf"sv, "application/octet-stream"sv },
    { "m1v"sv, "video/mpeg"sv },
    { "m21"sv, "application/mp21"sv },
    { "m2a"sv, "audio/mpeg"sv },
    { "m2v"sv, "video/mpeg"sv },
    { "m3a"sv, "audio/mpeg"sv },
    { "m4a"sv, "audio/mp4"sv },
    { "m4p"sv, "application/mp4"sv },
    { "ma"sv, "application/mathematica"sv },
    { "mads"sv, "application/mads+xml"sv },
    { "man"sv, "text/troff"sv },
    { "manifest"sv, "text/cache-manifest"sv },
    { "map"sv, "application/json"sv },
    { "mar"sv, "application/octet-stream"sv },
    { "markdown"sv, "text/markdown"sv },
    { "mathml"sv, "application/mathml+xml"sv },
    { "mb"sv, "application/mathematica"sv },
    { "mbox"sv, "application/mbox"sv },
    { "md"sv, "text/markdown"sv },
    { "me"sv, "text/troff"sv },
    { "mesh"sv, "model/mesh"sv },
    { "meta4"sv, "application/metalink4+xml"sv },
    { "metalink"sv, "application/metalink+xml"sv },
    { "mets"sv, "application/mets+xml"sv },
    { "mft"sv, "application/rpki-manifest"sv },
    { "mid"sv, "audio/midi"sv },
    { "midi"sv, "audio/midi"sv },
    { "mime"sv, "message/rfc822"sv },
    { "mj2"sv, "video/mj2"sv },
    { "mjp2"sv, "video/mj2"sv },
    { "mjs"sv, "application/javascript"sv },
    { "mml"sv, "text/mathml"sv },
    { "mods"sv, "application/mods+xml"sv },
    { "mov"sv, "video/quicktime"sv },
    { "mp2"sv, "audio/mpeg"sv },
    { "mp21"sv, "application/mp21"sv },
    { "mp2a"sv, "audio/mpeg"sv },
    { "mp3"sv, "audio/mpeg"sv },
    { "mp4"sv, "video/mp4"sv },
    { "mp4a"sv, "audio/mp4"sv },
    { "mp4s"sv, "application/mp4"sv },
    { "mp4v"sv, "video/mp4"sv },
    { "mpd"sv, "application/dash+xml"sv },
    { "mpe"sv, "video/mpeg"sv },
    { "mpeg"sv, "video/mpeg"sv },
    { "mpg"sv, "video/mpeg"sv },
    { "mpg4"sv, "video/mp4"sv },
    { "mpga"sv, "audio/mpeg"sv },
    { "mrc"sv, "application/marc"sv },
    { "mrcx"sv, "application/marcxml+xml"sv },
    { "ms"sv, "text/troff"sv },
    { "mscml"sv, "application/mediaservercontrol+xml"sv },
    { "msh"sv, "model/mesh"sv },
    { "msi"sv, "application/octet-stream"sv },
    { "msm"sv, "application/octet-stream"sv },
    { "msp"sv, "application/octet-stream"sv },
    { "mxf"sv, "application/mxf"sv },
    { "mxml"sv, "application/xv+xml"sv },
    { "n3"sv, "text/n3"sv },
    { "nb"sv, "application/mathematica"sv },
    { "oda"sv, "application/oda"sv },
    { "oga"sv, "audio/ogg"sv },
    { "ogg"sv, "audio/ogg"sv },
    { "ogv"sv, "video/ogg"sv },
    { "ogx"sv, "application/ogg"sv },
    { "omdoc"sv, "application/omdoc+xml"sv },
    { "onepkg"sv, "application/onenote"sv },
    { "onetmp"sv, "application/onenote"sv },
    { "onetoc"sv, "application/onenote"sv },
    { "onetoc2"sv, "application/onenote"sv },
    { "opf"sv, "application/oebps-package+xml"sv },
    { "otf"sv, "font/otf"sv },
    { "owl"sv, "application/rdf+xml"sv },
    { "oxps"sv, "application/oxps"sv },
    { "p10"sv, "application/pkcs10"sv },
    { "p7c"sv, "application/pkcs7-mime"sv },
    { "p7m"sv, "application/pkcs7-mime"sv },
    { "p7s"sv, "application/pkcs7-signature"sv },
    { "p8"sv, "application/pkcs8"sv },
    { "pdf"sv, "application/pdf"sv },
    { "pfr"sv, "application/font-tdpfr"sv },
    { "pgp"sv, "application/pgp-encrypted"sv },
    { "pkg"sv, "application/octet-stream"sv },
    { "pki"sv, "application/pkixcmp"sv },
    { "pkipath"sv, "application/pkix-pkipath"sv },
    { "pls"sv, "application/pls+xml"sv },
    { "png"sv, "image/png"sv },
    { "prf"sv, "application/pics-rules"sv },
    { "ps"sv, "application/postscript"sv },
    { "pskcxml"sv, "application/pskc+xml"sv },
    { "qt"sv, "video/quicktime"sv },
    { "raml"sv, "application/raml+yaml"sv },
    { "rdf"sv, "application/rdf+xml"sv },
    { "rif"sv, "application/reginfo+xml"sv },
    { "rl"sv, "application/resource-lists+xml"sv },
    { "rld"sv, "application/resource-lists-diff+xml"sv },
    { "rmi"sv, "audio/midi"sv },
    { "rnc"sv, "application/relax-ng-compact-syntax"sv },
    { "rng"sv, "application/xml"sv },
    { "roa"sv, "application/rpki-roa"sv },
    { "roff"sv, "text/troff"sv },
    { "rq"sv, "application/sparql-query"sv },
    { "rs"sv, "application/rls-services+xml"sv },
    { "rsd"sv, "application/rsd+xml"sv },
    { "rss"sv, "application/rss+xml"sv },
    { "rtf"sv, "application/rtf"sv },
    { "rtx"sv, "text/richtext"sv },
    { "s3m"sv, "audio/s3m"sv },
    { "sbml"sv, "application/sbml+xml"sv },
    { "scq"sv, "application/scvp-cv-request"sv },
    { "scs"sv, "application/scvp-cv-response"sv },
    { "sdp"sv, "application/sdp"sv },
    { "ser"sv, "application/java-serialized-object"sv },
    { "setpay"sv, "application/set-payment-initiation"sv },
    { "setreg"sv, "application/set-registration-initiation"sv },
    { "sgi"sv, "image/sgi"sv },
    { "sgm"sv, "text/sgml"sv },
    { "sgml"sv, "text/sgml"sv },
    { "shex"sv, "text/shex"sv },
    { "shf"sv, "application/shf+xml"sv },
    { "shtml"sv, "text/html"sv },
    { "sig"sv, "application/pgp-signature"sv },
    { "sil"sv, "audio/silk"sv },
    { "silo"sv, "model/mesh"sv },
    { "slim"sv, "text/slim"sv },
    { "slm"sv, "text/slim"sv },
    { "smi"sv, "application/smil+xml"sv },
    { "smil"sv, "application/smil+xml"sv },
    { "snd"sv, "audio/basic"sv },
    { "so"sv, "application/octet-stream"sv },
    { "spp"sv, "application/scvp-vp-response"sv },
    { "spq"sv, "application/scvp-vp-request"sv },
    { "spx"sv, "audio/ogg"sv },
    { "sru"sv, "application/sru+xml"sv },
    { "srx"sv, "application/sparql-results+xml"sv },
    { "ssdl"sv, "application/ssdl+xml"sv },
    { "ssml"sv, "application/ssml+xml"sv },
    { "stk"sv, "application/hyperstudio"sv },
    { "styl"sv, "text/stylus"sv },
    { "stylus"sv, "text/stylus"sv },
    { "svg"sv, "image/svg+xml"sv },
    { "svgz"sv, "image/svg+xml"sv },
    { "t"sv, "text/troff"sv },
    { "t38"sv, "image/t38"sv },
    { "tei"sv, "application/tei+xml"sv },
    { "teicorpus"sv, "application/tei+xml"sv },
    { "text"sv, "text/plain"sv },
    { "tfi"sv, "application/thraud+xml"sv },
    { "tfx"sv, "image/tiff-fx"sv },
    { "tif"sv, "image/tiff"sv },
    { "tiff"sv, "image/tiff"sv },
    { "tr"sv, "text/troff"sv },
    { "ts"sv, "video/mp2t"sv },
    { "tsd"sv, "application/timestamped-data"sv },
    { "tsv"sv, "text/tab-separated-values"sv },
    { "ttc"sv, "font/collection"sv },
    { "ttf"sv, "font/ttf"sv },
    { "ttl"sv, "text/turtle"sv },
    { "txt"sv, "text/plain"sv },
    { "u8dsn"sv, "message/global-delivery-status"sv },
    { "u8hdr"sv, "message/global-headers"sv },
    { "u8mdn"sv, "message/global-disposition-notification"sv },
    { "u8msg"sv, "message/global"sv },
    { "uri"sv, "text/uri-list"sv },
    { "uris"sv, "text/uri-list"sv },
    { "urls"sv, "text/uri-list"sv },
    { "vcard"sv, "text/vcard"sv },
    { "vrml"sv, "model/vrml"sv },
    { "vtt"sv, "text/vtt"sv },
    { "vxml"sv, "application/voicexml+xml"sv },
    { "war"sv, "application/java-archive"sv },
    { "wasm"sv, "application/wasm"sv },
    { "wav"sv, "audio/wav"sv },
    { "weba"sv, "audio/webm"sv },
    { "webm"sv, "video/webm"sv },
    { "webmanifest"sv, "application/manifest+json"sv },
    { "webp"sv, "image/webp"sv },
    { "wgt"sv, "application/widget"sv },
    { "wmf"sv, "image/wmf"sv },
    { "woff"sv, "font/woff"sv },
    { "woff2"sv, "font/woff2"sv },
    { "wrl"sv, "model/vrml"sv },
    { "wsdl"sv, "application/wsdl+xml"sv },
    { "wspolicy"sv, "application/wspolicy+xml"sv },
    { "x3d"sv, "model/x3d+xml"sv },
    { "x3db"sv, "model/x3d+binary"sv },
    { "x3dbz"sv, "model/x3d+binary"sv },
    { "x3dv"sv, "model/x3d+vrml"sv },
    { "x3dvz"sv, "model/x3d+vrml"sv },
    { "x3dz"sv, "model/x3d+xml"sv },
    { "xaml"sv, "application/xaml+xml"sv },
    { "xdf"sv, "application/xcap-diff+xml"sv },
    { "xdssc"sv, "application/dssc+xml"sv },
    { "xenc"sv, "application/xenc+xml"sv },
    { "xer"sv, "application/patch-ops-error+xml"sv },
    { "xht"sv, "application/xhtml+xml"sv },
    { "xhtml"sv, "application/xhtml+xml"sv },
    { "xhvml"sv, "application/xv+xml"sv },
    { "xm"sv, "audio/xm"sv },
    { "xml"sv, "application/xml"sv },
    { "xop"sv, "application/xop+xml"sv },
    { "xpl"sv, "application/xproc+xml"sv },
    { "xsd"sv, "application/xml"sv },
    { "xsl"sv, "application/xml"sv },
    { "xslt"sv, "application/xslt+xml"sv },
    { "xspf"sv, "application/xspf+xml"sv },
    { "xvm"sv, "application/xv+xml"sv },
    { "xvml"sv, "application/xv+xml"sv },
    { "yaml"sv, "text/yaml"sv },
    { "yang"sv, "application/yang"sv },
    { "yin"sv, "application/yin+xml"sv },
    { "yml"sv, "text/yaml"sv },
    { "zip"sv, "application/zip"sv },
};

constexpr const char* BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string encode_triple(uint32_t triple, uint8_t num_of_bits)
{
    uint8_t shift {};
    uint8_t chunk {};

    std::string result {};
    result.reserve(num_of_bits / 8);

    while (num_of_bits != 0) {
        // If we have more than 6 bits, we can use the last 6 bits of the triple.
        if (num_of_bits >= 6) {
            shift = num_of_bits - 6;
            chunk = (triple >> shift) & 0x3f;
            result += BASE64_CHARS[chunk];
            num_of_bits -= static_cast<uint8_t>(6);
        } else { // We don't have three bytes, so we have to pad the last byte.
            chunk = (triple << (6 - num_of_bits)) & 0x3f;
            result += BASE64_CHARS[chunk];
            num_of_bits = 0;
        }
    }

    return result;
}
} // namespace

namespace ekisocket::util {
bool CaseInsensitiveComp::operator()(std::string_view lkey, std::string_view rkey) const
{
    return std::lexicographical_compare(lkey.begin(), lkey.end(), rkey.begin(), rkey.end(),
        [](const unsigned char& l, const unsigned char& r) { return std::tolower(l) < std::tolower(r); });
}

std::string base64_encode(const uint8_t* input, size_t len)
{
    std::string encoded {};

    // This is how many bytes we're going to have.
    const auto total = (len % 3 == 0 ? len : (len + 3 - (len % 3))); // Ex: 12 for "Hello World".
    // This is how many bytes we have to make (subtracted from the ones we actually have).
    const auto padding = total - len; // Ex: 1 for "Hello World".

    // Reserving enough space. total * 4/3
    // x4 because we're going to have 4 characters for every 3 bytes.
    // x1/3 because every THREE characters.
    // Think of it as a rate of conversion. e.g. 4 characters per 3 bytes.
    encoded.reserve(total * 4 / 3);

    // Recording the number of bytes we have.
    uint8_t num_of_bytes {};
    // Recording the joined bytes.
    uint32_t triple {};

    // For every three bytes, concatenate them into 24 bits.
    for (size_t i {}; i < total; i += 3) { // Guaranteed to pass the last triple.
        if (i + 2 < (total - padding)) {
            triple |= input[i + 2];
            ++num_of_bytes;
        }
        if (i + 1 < (total - padding)) {
            triple |= static_cast<uint32_t>((input[i + 1] << (8 * num_of_bytes)));
            ++num_of_bytes;
        }

        triple |= static_cast<uint32_t>((input[i]
            << (8 * num_of_bytes))); // Mathematically, it is impossible for input[i] to be a padding character.
        ++num_of_bytes;
        encoded += encode_triple(triple, num_of_bytes * 8);
        num_of_bytes = 0;
        triple = 0;
    }
    // Append the padding characters.
    for (size_t i {}; i < padding; ++i) {
        encoded += '=';
    }
    return encoded;
}

std::string base64_encode(std::string_view input)
{
    return base64_encode(reinterpret_cast<const uint8_t*>(input.data()), input.size());
}

std::string compute_accept(const std::string& key)
{
    // The WebSocket key is computed as follows:
    // 1.  Append the string "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" to the end of the new header field value.
    // 2.  Compute the SHA1 hash of this modified header field value.
    // 3.  Base64 encode the SHA1 hash and replace the original header field value with this value.
    // The computed_accept value is used as the Sec-WebSocket-Accept header field value in the handshake response.

    // 1.
    const auto key_with_suffix = fmt::format("{}258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);
    // 2.
    std::array<uint8_t, SHA_DIGEST_LENGTH> hash {};
    SHA1(reinterpret_cast<const uint8_t*>(key_with_suffix.data()), key_with_suffix.length(), hash.data());
    // 3.
    return base64_encode(hash.data(), SHA_DIGEST_LENGTH);
}

std::string get_random_base64_from(uint32_t source_len)
{
    std::string ret {};

    // Must be rounded to a multiple of 3.
    const size_t rounded_bytes = (source_len % 3 == 0 ? source_len : (source_len + 3 - (source_len % 3)));
    // Must pad the remainder.
    const size_t padding = rounded_bytes - source_len;
    // Have to reserve the theoretical space for encoding.
    const size_t reserved_bytes = rounded_bytes * 4 / 3;

    ret.reserve(reserved_bytes);
    std::random_device rd {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dis(0, 63);

    // Append the random characters.
    for (uint32_t i {}; i < reserved_bytes - padding; ++i) {
        ret += BASE64_CHARS[static_cast<uint8_t>(dis(gen))];
    }
    // Append missing padding characters.
    for (uint32_t i {}; i < padding; ++i) {
        ret += '=';
    }

    return ret;
}

uint32_t get_random_number(uint32_t min, uint32_t max)
{
    std::random_device rd {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(min, max);
    return dis(gen);
}

bool iequals(const std::string& a, const std::string& b)
{
    if (a.length() != b.length()) {
        return false;
    }
    for (size_t i {}; i < a.length(); ++i) {
        if (std::tolower(a[i]) != std::tolower(b[i])) {
            return false;
        }
    }
    return true;
}

std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return std::isspace(ch) != 0; }));
    return s;
}

std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return std::isspace(ch) != 0; }).base(), s.end());
    return s;
}

std::string& trim(std::string& s) { return ltrim(rtrim(s)); }

std::vector<std::string> split(std::string_view s, std::string_view delimiter)
{
    std::vector<std::string> ret {};
    size_t pos {};
    size_t prev {};

    while ((pos = s.find(delimiter, prev)) != std::string::npos) {
        ret.emplace_back(s.substr(prev, pos - prev));
        prev = pos + delimiter.length();
    }

    ret.emplace_back(s.substr(prev));
    return ret;
}

std::string join(const std::vector<std::string>& v, std::string_view delimiter)
{
    std::string ret {};

    for (size_t i {}; i < v.size(); ++i) {
        ret += v[i];
        if (i != v.size() - 1) {
            ret += delimiter;
        }
    }

    return ret;
}

bool is_number(std::string_view s) { return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit); }

std::string create_boundary()
{
    // A boundary string cannot exceed 70 characters, and must consist only of printable ASCII characters.

    // 1.  Choose a random string of length between 1 and 70 characters.
    const auto rng = get_random_number(1, 70);
    std::string ret {};
    ret.reserve(rng);

    // 2.  Choose a random printable ASCII character.
    // The printable ASCII characters are the characters from the range 32 to 127, inclusive.
    std::random_device rd {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution dis(32, 127);

    // 3.  Append the character to the string.
    for (size_t i {}; i < rng; ++i) {
        ret += static_cast<char>(dis(gen));
    }

    return ret;
}

std::string create_multipart_form_data(const std::string& key, std::string_view value, const std::string& boundary)
{
    return fmt::format("--{}\r\n"
                       "Content-Disposition: form-data; name=\"{}\"\r\n"
                       "{}",
        boundary, key, value);
}

std::string create_multipart_form_data(
    const std::vector<std::pair<std::string, std::string>>& key_value_pairs, const std::string& boundary)
{
    std::string ret {};
    for (const auto& [key, value] : key_value_pairs) {
        ret += create_multipart_form_data(key, value, boundary);
        ret += "\r\n";
    }
    return ret;
}

std::string create_multipart_form_data_file(
    const std::string& name, std::string_view file_contents, const std::string& filename, const std::string& boundary)
{
    auto ret = fmt::format("--{}\r\r"
                           "Content-Disposition: form-data; name=\"{}\"; filename=\"{}\"\r\n",
        boundary, name, filename);

    // Determine the content type.
    const std::string_view content_type = std::invoke([&filename] {
        // If the filename has an extension. (e.g. "foo.jpg"), we can determine the content type, from that.
        const auto dot = filename.find_last_of('.');

        if (dot == std::string::npos) {
            return "application/octet-stream"sv;
        }

        // Compare the possible extensions.
        if (const auto ext = filename.substr(dot + 1); CONTENT_TYPES.contains(ext)) {
            return CONTENT_TYPES.at(ext);
        }

        return "application/octet-stream"sv;
    });

    ret += fmt::format("Content-Type: {}\r\n\r\n{}", content_type, file_contents);
    return ret;
}

std::string create_application_x_www_form_urlencoded(const std::string& key, const std::string& value)
{
    return fmt::format("{}={}&", key, value);
}
} // namespace ekisocket::util

#include "SoapyPlutoSDR.hpp"
#include <SoapySDR/Registry.hpp>
#include <iostream>
#include <sstream>

static std::vector<SoapySDR::Kwargs> results;
static std::vector<SoapySDR::Kwargs> find_PlutoSDR(const SoapySDR::Kwargs &args)
{

    if (!results.empty())
        return results;

    ssize_t ret = 0;
    struct iio_context *ctx = nullptr;
    struct iio_scan *scan_ctx = nullptr;
    SoapySDR::Kwargs options;
    // Backends can error, scan each one individually
    std::vector<std::string> backends = {"local", "usb", "ip"};
    for (std::vector<std::string>::iterator it = backends.begin(); it != backends.end(); it++) {

        scan_ctx = iio_scan(NULL, it->c_str());
        if (scan_ctx == nullptr) {
            SoapySDR_logf(SOAPY_SDR_WARNING, "Unable to setup %s scan\n", it->c_str());
            continue;
        }

        ret = iio_scan_get_results_count(scan_ctx);
        printf("found %i devices\n", ret);
        if (ret < 0) {
            SoapySDR_logf(SOAPY_SDR_WARNING, "Unable to scan %s: %li\n", it->c_str(), (long)ret);
            iio_scan_destroy(scan_ctx);
            continue;
        }

        options["device"] = "PlutoSDR";

        if (ret == 0) {
            iio_scan_destroy(scan_ctx);

            // no devices discovered, the user must specify a hostname
            if (args.count("hostname") == 0)
                continue;

            // try to connect at the specified hostname
            ctx = iio_create_context(NULL, args.at("hostname").c_str());
            if (ctx == nullptr)
                continue; // failed to connect
            options["hostname"] = args.at("hostname");

            std::ostringstream label_str;
            label_str << options["device"] << " #0 " << options["hostname"];
            options["label"] = label_str.str();

            results.push_back(options);
            if (ctx != nullptr)
                iio_context_destroy(ctx);
        }
        else {
            for (int i = 0; i < ret; i++) {
                ctx = iio_create_context(NULL, iio_scan_get_uri(scan_ctx, i));
                if (ctx != nullptr) {
                    options["uri"] = std::string(iio_scan_get_uri(scan_ctx, i));

                    std::ostringstream label_str;
                    label_str << options["device"] << " #" << i << " " << options["uri"];
                    options["label"] = label_str.str();

                    results.push_back(options);
                    if (ctx != nullptr)
                        iio_context_destroy(ctx);
                }
            }
            iio_scan_destroy(scan_ctx);
        }
    }
    return results;
}

static SoapySDR::Device *make_PlutoSDR(const SoapySDR::Kwargs &args)
{
    return new SoapyPlutoSDR(args);
}

static SoapySDR::Registry register_plutosdr("plutosdr", &find_PlutoSDR, &make_PlutoSDR,
                                            SOAPY_SDR_ABI_VERSION);

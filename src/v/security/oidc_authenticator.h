/*
 * Copyright 2023 Redpanda Data, Inc.
 *
 * Licensed as a Redpanda Enterprise file under the Redpanda Community
 * License (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 * https://github.com/redpanda-data/redpanda/blob/master/licenses/rcl.md
 */
#pragma once
#include "outcome.h"
#include "security/acl.h"
#include "security/fwd.h"
#include "security/sasl_authentication.h"

#include <seastar/core/lowres_clock.hh>

#include <chrono>

namespace security::oidc {

result<acl_principal> authenticate(
  jws const& jws,
  verifier const& verifier,
  std::string_view issuer,
  std::string_view audience,
  std::chrono::seconds clock_skew_tolerance,
  ss::lowres_system_clock::time_point now);

result<acl_principal> authenticate(
  jwt const& jwt,
  std::string_view issuer,
  std::string_view audience,
  std::chrono::seconds clock_skew_tolerance,
  ss::lowres_system_clock::time_point now);

class authenticator {
public:
    explicit authenticator(service& service);
    authenticator(authenticator&&) = default;
    authenticator(authenticator const&) = delete;
    authenticator& operator=(authenticator&&) = delete;
    authenticator& operator=(authenticator const&) = delete;
    ~authenticator();

    result<acl_principal> authenticate(std::string_view bearer_token);

private:
    class impl;
    std::unique_ptr<impl> _impl;
};

class sasl_authenticator final : public sasl_mechanism {
public:
    enum class state { init = 0, complete, failed };
    static constexpr const char* name = "OAUTHBEARER";

    explicit sasl_authenticator(oidc::service& service);
    sasl_authenticator(sasl_authenticator&&) = default;
    sasl_authenticator(sasl_authenticator const&) = delete;
    sasl_authenticator& operator=(sasl_authenticator&&) = delete;
    sasl_authenticator& operator=(sasl_authenticator const&) = delete;
    ~sasl_authenticator() override;

    ss::future<result<bytes>> authenticate(bytes) override;

    bool complete() const override { return _state == state::complete; }
    bool failed() const override { return _state == state::failed; }

    const security::acl_principal& principal() const override {
        return _principal;
    }

private:
    friend std::ostream&
    operator<<(std::ostream& os, sasl_authenticator::state const s);

    authenticator _authenticator;
    security::acl_principal _principal;
    state _state{state::init};
};

} // namespace security::oidc

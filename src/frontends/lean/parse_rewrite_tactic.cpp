/*
Copyright (c) 2015 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include "library/tactic/rewrite_tactic.h"
#include "frontends/lean/parser.h"
#include "frontends/lean/tokens.h"
#include "frontends/lean/parse_tactic_location.h"

namespace lean {
static optional<expr> parse_pattern(parser & p) {
    if (p.curr_is_token(get_lcurly_tk())) {
        p.next();
        expr r = p.parse_expr();
        p.check_token_next(get_rcurly_tk(), "invalid rewrite pattern, '}' expected");
        return some_expr(r);
    } else {
        return none_expr();
    }
}

static expr parse_rule(parser & p) {
    if (p.curr_is_token(get_lparen_tk())) {
        p.next();
        expr r = p.parse_expr();
        p.check_token_next(get_rparen_tk(), "invalid rewrite pattern, ')' expected");
        return r;
    } else {
        return p.parse_id();
    }
}

static expr parse_rewrite_unfold_core(parser & p) {
    buffer<name> to_unfold;
    if (p.curr_is_token(get_lcurly_tk())) {
        p.next();
        while (true) {
            to_unfold.push_back(p.check_constant_next("invalid unfold rewrite step, identifier expected"));
            if (!p.curr_is_token(get_comma_tk()))
                break;
            p.next();
        }
        p.check_token_next(get_rcurly_tk(), "invalid unfold rewrite step, ',' or '}' expected");
    } else {
        to_unfold.push_back(p.check_constant_next("invalid unfold rewrite step, identifier or '{' expected"));
    }
    location loc = parse_tactic_location(p);
    return mk_rewrite_unfold(to_list(to_unfold), loc);
}

static expr parse_rewrite_unfold(parser & p) {
    lean_assert(p.curr_is_token(get_up_tk()) || p.curr_is_token(get_caret_tk()));
    p.next();
    return parse_rewrite_unfold_core(p);
}

expr parse_rewrite_element(parser & p) {
    if (p.curr_is_token(get_up_tk()) || p.curr_is_token(get_caret_tk()))
        return parse_rewrite_unfold(p);
    if (p.curr_is_token(get_down_tk())) {
        p.next();
        expr e = p.parse_expr();
        location loc = parse_tactic_location(p);
        return mk_rewrite_fold(e, loc);
    }
    bool symm = false;
    if (p.curr_is_token(get_sub_tk())) {
        p.next();
        symm = true;
    }
    if (p.curr_is_numeral()) {
        unsigned n = p.parse_small_nat();
        if (p.curr_is_token(get_greater_tk())) {
            p.next();
            optional<expr> pat = parse_pattern(p);
            expr H = parse_rule(p);
            location loc = parse_tactic_location(p);
            return mk_rewrite_at_most_n(pat, H, symm, n, loc);
        } else {
            optional<expr> pat = parse_pattern(p);
            expr H = parse_rule(p);
            location loc = parse_tactic_location(p);
            return mk_rewrite_exactly_n(pat, H, symm, n, loc);
        }
    } else if (p.curr_is_token(get_star_tk())) {
        p.next();
        optional<expr> pat = parse_pattern(p);
        expr H = parse_rule(p);
        location loc = parse_tactic_location(p);
        return mk_rewrite_zero_or_more(pat, H, symm, loc);
    } else if (p.curr_is_token(get_plus_tk())) {
        p.next();
        optional<expr> pat = parse_pattern(p);
        expr H = parse_rule(p);
        location loc = parse_tactic_location(p);
        return mk_rewrite_one_or_more(pat, H, symm, loc);
    } else if (p.curr_is_token(get_triangle_tk()) || p.curr_is_token(get_greater_tk())) {
        p.next();
        if (p.curr_is_token(get_star_tk())) {
            p.next();
            location loc = parse_tactic_location(p);
            return mk_rewrite_reduce(loc);
        } else {
            expr e = p.parse_expr();
            location loc = parse_tactic_location(p);
            return mk_rewrite_reduce_to(e, loc);
        }
    } else {
        optional<expr> pat = parse_pattern(p);
        expr H = parse_rule(p);
        location loc = parse_tactic_location(p);
        return mk_rewrite_once(pat, H, symm, loc);
    }
}

expr parse_rewrite_tactic(parser & p) {
    buffer<expr> elems;
    bool lbraket = p.curr_is_token(get_lbracket_tk());
    if (lbraket || p.curr_is_token(get_langle_tk())) {
        p.next();
        while (true) {
            auto pos = p.pos();
            elems.push_back(p.save_pos(parse_rewrite_element(p), pos));
            if (!p.curr_is_token(get_comma_tk()))
                break;
            p.next();
        }
        if (lbraket)
            p.check_token_next(get_rbracket_tk(), "invalid rewrite tactic, ']' expected");
        else
            p.check_token_next(get_rangle_tk(), "invalid rewrite tactic, '⟩' expected");
    } else {
        auto pos = p.pos();
        elems.push_back(p.save_pos(parse_rewrite_element(p), pos));
    }
    return mk_rewrite_tactic_expr(elems);
}

expr parse_esimp_tactic(parser & p) {
    buffer<expr> elems;
    if (p.curr_is_token(get_up_tk()) || p.curr_is_token(get_caret_tk())) {
        elems.push_back(parse_rewrite_unfold(p));
    } else if (p.curr_is_token(get_lcurly_tk())) {
        elems.push_back(parse_rewrite_unfold_core(p));
    } else {
        location loc = parse_tactic_location(p);
        elems.push_back(mk_rewrite_reduce(loc));
    }
    return mk_rewrite_tactic_expr(elems);
}
}

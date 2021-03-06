/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/chain/committee_member_evaluator.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>

#include <fc/smart_ref_impl.hpp>

namespace graphene { namespace chain {

void_result committee_member_create_evaluator::do_evaluate( const committee_member_create_operation& op )
{ try {
   FC_ASSERT(db().get(op.committee_member_account).is_lifetime_member());
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type committee_member_create_evaluator::do_apply( const committee_member_create_operation& op )
{ try {
   vote_id_type vote_id;
   db().modify(db().get_global_properties(), [&vote_id](global_property_object& p) {
      vote_id = get_next_vote_id(p, vote_id_type::committee);
   });

   const auto& new_del_object = db().create<committee_member_object>( [&]( committee_member_object& obj ){
         obj.committee_member_account   = op.committee_member_account;
         obj.vote_id            = vote_id;
         obj.url                = op.url;
   });
   return new_del_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_update_evaluator::do_evaluate( const committee_member_update_operation& op )
{ try {
   FC_ASSERT(db().get(op.committee_member).committee_member_account == op.committee_member_account);
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_update_evaluator::do_apply( const committee_member_update_operation& op )
{ try {
   database& _db = db();
   _db.modify(
      _db.get(op.committee_member),
      [&]( committee_member_object& com )
      {
         if( op.new_url.valid() )
            com.url = *op.new_url;
      });
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result committee_member_update_global_parameters_evaluator::do_evaluate(const committee_member_update_global_parameters_operation& o)
{ try {
   FC_ASSERT(trx_state->_is_proposed_trx);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result committee_member_update_global_parameters_evaluator::do_apply(const committee_member_update_global_parameters_operation& o)
{ try {
   db().modify(db().get_global_properties(), [&o](global_property_object& p) {
      p.pending_parameters = o.new_parameters;
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result committee_member_issue_construction_capital_evaluator::do_evaluate(const committee_member_issue_construction_capital_operation& o)
{ try {
   FC_ASSERT(trx_state->_is_proposed_trx);
   FC_ASSERT(
         time_point(db().head_block_time()) <= time_point::from_iso_string("2018-07-01T00:00:00"),
         "construction capital can only be issued before 2018-07-01T00:00:00, now is: ${now}",
         ("now", db().head_block_time())
   );
   FC_ASSERT(
         o.amount <= db().get_balance(GRAPHENE_CONSTRUCTION_CAPITAL_ACCOUNT, asset_id_type(0)).amount,
         "transfer amount:${amount} should not greater than GRAPHENE_CONSTRUCTION_CAPITAL_ACCOUNT's balance: ${cca}",
         ("amount", o.amount)
         ("cca", db().get_balance(GRAPHENE_CONSTRUCTION_CAPITAL_ACCOUNT, asset_id_type(0)).amount)
   );
   FC_ASSERT(
         db().find(o.receiver) != nullptr,
         "receiver: ${receiver} not found",
         ("receiver", o.receiver)
   );
   FC_ASSERT(
         o.amount > o.fee.amount,
         "increase_supply should issue ${fee} at least, only ${amt} specified",
         ("fee", o.fee.amount)
         ("amt", o.amount)
   );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result committee_member_issue_construction_capital_evaluator::do_apply(const committee_member_issue_construction_capital_operation& o)
{ try {
   db().adjust_balance(o.receiver, asset(o.amount, asset_id_type(0)));
   db().adjust_balance(GRAPHENE_CONSTRUCTION_CAPITAL_ACCOUNT, -asset(o.amount, asset_id_type(0)));
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result committee_member_grant_instant_payback_evaluator::do_evaluate(const committee_member_grant_instant_payback_operation& o)
{ try {
   FC_ASSERT(trx_state->_is_proposed_trx);
   FC_ASSERT(
         db().find(o.account_id) != nullptr,
         "account: ${account} not found",
         ("account", o.account_id)
   );
   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result committee_member_grant_instant_payback_evaluator::do_apply(const committee_member_grant_instant_payback_operation& o)
{ try {
   auto acc_obj = (const account_object *)db().find(o.account_id);
   db().modify(*acc_obj, [&](account_object &obj) {
      if (o.grant) {
         obj.instant_payback_expiration_date = db().head_block_time() + fc::days(365);
      } else {
         obj.instant_payback_expiration_date = time_point::min();
      }
   });
return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

} } // graphene::chain

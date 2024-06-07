#include "emitterstate.h"

#include <limits>

#include "exceptions.h"  // IWYU pragma: keep

namespace YAML {
EmitterState::EmitterState()
    : m_isGood(true),
      m_lastError{},
      // default global manipulators
      m_charset(EmitterManip::EmitNonAscii),
      m_strFmt(EmitterManip::Auto),
      m_boolFmt(EmitterManip::TrueFalseBool),
      m_boolLengthFmt(EmitterManip::LongBool),
      m_boolCaseFmt(EmitterManip::LowerCase),
      m_nullFmt(EmitterManip::TildeNull),
      m_intFmt(EmitterManip::Dec),
      m_indent(2),
      m_preCommentIndent(2),
      m_postCommentIndent(1),
      m_seqFmt(EmitterManip::Block),
      m_mapFmt(EmitterManip::Block),
      m_mapKeyFmt(EmitterManip::Auto),
      m_floatPrecision(std::numeric_limits<float>::max_digits10),
      m_doublePrecision(std::numeric_limits<double>::max_digits10),
      //
      m_modifiedSettings{},
      m_globalModifiedSettings{},
      m_groups{},
      m_curIndent(0),
      m_hasAnchor(false),
      m_hasAlias(false),
      m_hasTag(false),
      m_hasNonContent(false),
      m_docCount(0) {}

EmitterState::~EmitterState() = default;

// SetLocalValue
// . We blindly tries to set all possible formatters to this value
// . Only the ones that make sense will be accepted
void EmitterState::SetLocalValue(EmitterManip value) {
  SetOutputCharset(value, FmtScope::Local);
  SetStringFormat(value, FmtScope::Local);
  SetBoolFormat(value, FmtScope::Local);
  SetBoolCaseFormat(value, FmtScope::Local);
  SetBoolLengthFormat(value, FmtScope::Local);
  SetNullFormat(value, FmtScope::Local);
  SetIntFormat(value, FmtScope::Local);
  SetFlowType(GroupType::Seq, value, FmtScope::Local);
  SetFlowType(GroupType::Map, value, FmtScope::Local);
  SetMapKeyFormat(value, FmtScope::Local);
}

void EmitterState::SetAnchor() { m_hasAnchor = true; }

void EmitterState::SetAlias() { m_hasAlias = true; }

void EmitterState::SetTag() { m_hasTag = true; }

void EmitterState::SetNonContent() { m_hasNonContent = true; }

void EmitterState::SetLongKey() {
  assert(!m_groups.empty());
  if (m_groups.empty()) {
    return;
  }

  assert(m_groups.back()->type == GroupType::Map);
  m_groups.back()->longKey = true;
}

void EmitterState::ForceFlow() {
  assert(!m_groups.empty());
  if (m_groups.empty()) {
    return;
  }

  m_groups.back()->flowType = FlowType::Flow;
}

void EmitterState::StartedNode() {
  if (m_groups.empty()) {
    m_docCount++;
  } else {
    m_groups.back()->childCount++;
    if (m_groups.back()->childCount % 2 == 0) {
      m_groups.back()->longKey = false;
    }
  }

  m_hasAnchor = false;
  m_hasAlias = false;
  m_hasTag = false;
  m_hasNonContent = false;
}

EmitterNodeType EmitterState::NextGroupType(
    GroupType type) const {
  if (type == GroupType::Seq) {
    if (GetFlowType(type) == EmitterManip::Block)
      return EmitterNodeType::BlockSeq;
    return EmitterNodeType::FlowSeq;
  }

  if (GetFlowType(type) == EmitterManip::Block)
    return EmitterNodeType::BlockMap;
  return EmitterNodeType::FlowMap;
}

void EmitterState::StartedDoc() {
  m_hasAnchor = false;
  m_hasTag = false;
  m_hasNonContent = false;
}

void EmitterState::EndedDoc() {
  m_hasAnchor = false;
  m_hasTag = false;
  m_hasNonContent = false;
}

void EmitterState::StartedScalar() {
  StartedNode();
  ClearModifiedSettings();
}

void EmitterState::StartedGroup(GroupType type) {
  StartedNode();

  const std::size_t lastGroupIndent =
      (m_groups.empty() ? 0 : m_groups.back()->indent);
  m_curIndent += lastGroupIndent;

  // TODO: Create move constructors for settings types to simplify transfer
  std::unique_ptr<Group> pGroup(new Group(type));

  // transfer settings (which last until this group is done)
  //
  // NB: if pGroup->modifiedSettings == m_modifiedSettings,
  // m_modifiedSettings is not changed!
  pGroup->modifiedSettings = std::move(m_modifiedSettings);

  // set up group
  if (GetFlowType(type) == EmitterManip::Block) {
    pGroup->flowType = FlowType::Block;
  } else {
    pGroup->flowType = FlowType::Flow;
  }
  pGroup->indent = GetIndent();

  m_groups.push_back(std::move(pGroup));
}

void EmitterState::EndedGroup(GroupType type) {
  if (m_groups.empty()) {
    if (type == GroupType::Seq) {
      return SetError(ErrorMsg::UNEXPECTED_END_SEQ);
    }
    return SetError(ErrorMsg::UNEXPECTED_END_MAP);
  }

  if (m_hasTag) {
    SetError(ErrorMsg::INVALID_TAG);
  }
  if (m_hasAnchor) {
    SetError(ErrorMsg::INVALID_ANCHOR);
  }

  // get rid of the current group
  {
    std::unique_ptr<Group> pFinishedGroup = std::move(m_groups.back());
    m_groups.pop_back();
    if (pFinishedGroup->type != type) {
      return SetError(ErrorMsg::UNMATCHED_GROUP_TAG);
    }
  }

  // reset old settings
  std::size_t lastIndent = (m_groups.empty() ? 0 : m_groups.back()->indent);
  assert(m_curIndent >= lastIndent);
  m_curIndent -= lastIndent;

  // some global settings that we changed may have been overridden
  // by a local setting we just popped, so we need to restore them
  m_globalModifiedSettings.restore();

  ClearModifiedSettings();
  m_hasAnchor = false;
  m_hasTag = false;
  m_hasNonContent = false;
}

EmitterNodeType EmitterState::CurGroupNodeType() const {
  if (m_groups.empty()) {
    return EmitterNodeType::NoType;
  }

  return m_groups.back()->NodeType();
}

GroupType EmitterState::CurGroupType() const {
  return m_groups.empty() ? GroupType::NoType : m_groups.back()->type;
}

FlowType EmitterState::CurGroupFlowType() const {
  return m_groups.empty() ? FlowType::NoType : m_groups.back()->flowType;
}

std::size_t EmitterState::CurGroupIndent() const {
  return m_groups.empty() ? 0 : m_groups.back()->indent;
}

std::size_t EmitterState::CurGroupChildCount() const {
  return m_groups.empty() ? m_docCount : m_groups.back()->childCount;
}

bool EmitterState::CurGroupLongKey() const {
  return m_groups.empty() ? false : m_groups.back()->longKey;
}

std::size_t EmitterState::LastIndent() const {
  if (m_groups.size() <= 1) {
    return 0;
  }

  return m_curIndent - m_groups[m_groups.size() - 2]->indent;
}

void EmitterState::ClearModifiedSettings() { m_modifiedSettings.clear(); }

void EmitterState::RestoreGlobalModifiedSettings() {
  m_globalModifiedSettings.restore();
}

bool EmitterState::SetOutputCharset(EmitterManip value,
                                    FmtScope scope) {
  switch (value) {
    case EmitterManip::EmitNonAscii:
    case EmitterManip::EscapeNonAscii:
    case EmitterManip::EscapeAsJson:
      _Set(m_charset, value, scope);
      return true;
    default:
      return false;
  }
}

bool EmitterState::SetStringFormat(EmitterManip value, FmtScope scope) {
  switch (value) {
    case EmitterManip::Auto:
    case EmitterManip::SingleQuoted:
    case EmitterManip::DoubleQuoted:
    case EmitterManip::Literal:
      _Set(m_strFmt, value, scope);
      return true;
    default:
      return false;
  }
}

bool EmitterState::SetBoolFormat(EmitterManip value, FmtScope scope) {
  switch (value) {
    case EmitterManip::OnOffBool:
    case EmitterManip::TrueFalseBool:
    case EmitterManip::YesNoBool:
      _Set(m_boolFmt, value, scope);
      return true;
    default:
      return false;
  }
}

bool EmitterState::SetBoolLengthFormat(EmitterManip value,
                                       FmtScope scope) {
  switch (value) {
    case EmitterManip::LongBool:
    case EmitterManip::ShortBool:
      _Set(m_boolLengthFmt, value, scope);
      return true;
    default:
      return false;
  }
}

bool EmitterState::SetBoolCaseFormat(EmitterManip value,
                                     FmtScope scope) {
  switch (value) {
    case EmitterManip::UpperCase:
    case EmitterManip::LowerCase:
    case EmitterManip::CamelCase:
      _Set(m_boolCaseFmt, value, scope);
      return true;
    default:
      return false;
  }
}

bool EmitterState::SetNullFormat(EmitterManip value, FmtScope scope) {
  switch (value) {
    case EmitterManip::LowerNull:
    case EmitterManip::UpperNull:
    case EmitterManip::CamelNull:
    case EmitterManip::TildeNull:
      _Set(m_nullFmt, value, scope);
      return true;
    default:
      return false;
  }
}

bool EmitterState::SetIntFormat(EmitterManip value, FmtScope scope) {
  switch (value) {
    case EmitterManip::Dec:
    case EmitterManip::Hex:
    case EmitterManip::Oct:
      _Set(m_intFmt, value, scope);
      return true;
    default:
      return false;
  }
}

bool EmitterState::SetIndent(std::size_t value, FmtScope scope) {
  if (value <= 1)
    return false;

  _Set(m_indent, value, scope);
  return true;
}

bool EmitterState::SetPreCommentIndent(std::size_t value,
                                       FmtScope scope) {
  if (value == 0)
    return false;

  _Set(m_preCommentIndent, value, scope);
  return true;
}

bool EmitterState::SetPostCommentIndent(std::size_t value,
                                        FmtScope scope) {
  if (value == 0)
    return false;

  _Set(m_postCommentIndent, value, scope);
  return true;
}

bool EmitterState::SetFlowType(GroupType groupType, EmitterManip value,
                               FmtScope scope) {
  switch (value) {
    case EmitterManip::Block:
    case EmitterManip::Flow:
      _Set(groupType == GroupType::Seq ? m_seqFmt : m_mapFmt, value, scope);
      return true;
    default:
      return false;
  }
}

EmitterManip EmitterState::GetFlowType(GroupType groupType) const {
  // force flow style if we're currently in a flow
  if (CurGroupFlowType() == FlowType::Flow)
    return EmitterManip::Flow;

  // otherwise, go with what's asked of us
  return (groupType == GroupType::Seq ? m_seqFmt.get() : m_mapFmt.get());
}

bool EmitterState::SetMapKeyFormat(EmitterManip value, FmtScope scope) {
  switch (value) {
    case EmitterManip::Auto:
    case EmitterManip::LongKey:
      _Set(m_mapKeyFmt, value, scope);
      return true;
    default:
      return false;
  }
}

bool EmitterState::SetFloatPrecision(std::size_t value, FmtScope scope) {
  if (value > std::numeric_limits<float>::max_digits10)
    return false;
  _Set(m_floatPrecision, value, scope);
  return true;
}

bool EmitterState::SetDoublePrecision(std::size_t value,
                                      FmtScope scope) {
  if (value > std::numeric_limits<double>::max_digits10)
    return false;
  _Set(m_doublePrecision, value, scope);
  return true;
}
}  // namespace YAML

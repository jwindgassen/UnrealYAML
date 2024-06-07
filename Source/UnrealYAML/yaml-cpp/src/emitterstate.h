#ifndef EMITTERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define EMITTERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "setting.h"
#include "emitterdef.h"
#include "emittermanip.h"

#include <cassert>
#include <memory>
#include <stack>
#include <stdexcept>
#include <vector>

namespace YAML {
enum class FmtScope {Local, Global};
  
enum class GroupType {NoType, Seq, Map};
  
enum class FlowType {NoType, Flow, Block};

class EmitterState {
 public:
  EmitterState();
  ~EmitterState();

  // basic state checking
  bool good() const { return m_isGood; }
  const std::string GetLastError() const { return m_lastError; }
  void SetError(const std::string& error) {
    m_isGood = false;
    m_lastError = error;
  }

  // node handling
  void SetAnchor();
  void SetAlias();
  void SetTag();
  void SetNonContent();
  void SetLongKey();
  void ForceFlow();
  void StartedDoc();
  void EndedDoc();
  void StartedScalar();
  void StartedGroup(GroupType type);
  void EndedGroup(GroupType type);

  EmitterNodeType NextGroupType(GroupType type) const;
  EmitterNodeType CurGroupNodeType() const;

  GroupType CurGroupType() const;
  FlowType CurGroupFlowType() const;
  std::size_t CurGroupIndent() const;
  std::size_t CurGroupChildCount() const;
  bool CurGroupLongKey() const;

  std::size_t LastIndent() const;
  std::size_t CurIndent() const { return m_curIndent; }
  bool HasAnchor() const { return m_hasAnchor; }
  bool HasAlias() const { return m_hasAlias; }
  bool HasTag() const { return m_hasTag; }
  bool HasBegunNode() const {
    return m_hasAnchor || m_hasTag || m_hasNonContent;
  }
  bool HasBegunContent() const { return m_hasAnchor || m_hasTag; }

  void ClearModifiedSettings();
  void RestoreGlobalModifiedSettings();

  // formatters
  void SetLocalValue(EmitterManip value);

  bool SetOutputCharset(EmitterManip value, FmtScope scope);
  EmitterManip GetOutputCharset() const { return m_charset.get(); }

  bool SetStringFormat(EmitterManip value, FmtScope scope);
  EmitterManip GetStringFormat() const { return m_strFmt.get(); }

  bool SetBoolFormat(EmitterManip value, FmtScope scope);
  EmitterManip GetBoolFormat() const { return m_boolFmt.get(); }

  bool SetBoolLengthFormat(EmitterManip value, FmtScope scope);
  EmitterManip GetBoolLengthFormat() const { return m_boolLengthFmt.get(); }

  bool SetBoolCaseFormat(EmitterManip value, FmtScope scope);
  EmitterManip GetBoolCaseFormat() const { return m_boolCaseFmt.get(); }

  bool SetNullFormat(EmitterManip value, FmtScope scope);
  EmitterManip GetNullFormat() const { return m_nullFmt.get(); }

  bool SetIntFormat(EmitterManip value, FmtScope scope);
  EmitterManip GetIntFormat() const { return m_intFmt.get(); }

  bool SetIndent(std::size_t value, FmtScope scope);
  std::size_t GetIndent() const { return m_indent.get(); }

  bool SetPreCommentIndent(std::size_t value, FmtScope scope);
  std::size_t GetPreCommentIndent() const { return m_preCommentIndent.get(); }
  bool SetPostCommentIndent(std::size_t value, FmtScope scope);
  std::size_t GetPostCommentIndent() const { return m_postCommentIndent.get(); }

  bool SetFlowType(GroupType groupType, EmitterManip value,
                   FmtScope scope);
  EmitterManip GetFlowType(GroupType groupType) const;

  bool SetMapKeyFormat(EmitterManip value, FmtScope scope);
  EmitterManip GetMapKeyFormat() const { return m_mapKeyFmt.get(); }

  bool SetFloatPrecision(std::size_t value, FmtScope scope);
  std::size_t GetFloatPrecision() const { return m_floatPrecision.get(); }
  bool SetDoublePrecision(std::size_t value, FmtScope scope);
  std::size_t GetDoublePrecision() const { return m_doublePrecision.get(); }

 private:
  template <typename T>
  void _Set(Setting<T>& fmt, T value, FmtScope scope);

  void StartedNode();

 private:
  // basic state ok?
  bool m_isGood;
  std::string m_lastError;

  // other state
  Setting<EmitterManip> m_charset;
  Setting<EmitterManip> m_strFmt;
  Setting<EmitterManip> m_boolFmt;
  Setting<EmitterManip> m_boolLengthFmt;
  Setting<EmitterManip> m_boolCaseFmt;
  Setting<EmitterManip> m_nullFmt;
  Setting<EmitterManip> m_intFmt;
  Setting<std::size_t> m_indent;
  Setting<std::size_t> m_preCommentIndent, m_postCommentIndent;
  Setting<EmitterManip> m_seqFmt;
  Setting<EmitterManip> m_mapFmt;
  Setting<EmitterManip> m_mapKeyFmt;
  Setting<std::size_t> m_floatPrecision;
  Setting<std::size_t> m_doublePrecision;

  SettingChanges m_modifiedSettings;
  SettingChanges m_globalModifiedSettings;

  struct Group {
    explicit Group(GroupType type_)
        : type(type_),
          flowType{},
          indent(0),
          childCount(0),
          longKey(false),
          modifiedSettings{} {}

    GroupType type;
    FlowType flowType;
    std::size_t indent;
    std::size_t childCount;
    bool longKey;

    SettingChanges modifiedSettings;

    EmitterNodeType NodeType() const {
      if (type == GroupType::Seq) {
        if (flowType == FlowType::Flow)
          return EmitterNodeType::FlowSeq;
        else
          return EmitterNodeType::BlockSeq;
      } else {
        if (flowType == FlowType::Flow)
          return EmitterNodeType::FlowMap;
        else
          return EmitterNodeType::BlockMap;
      }
    }
  };

  std::vector<std::unique_ptr<Group>> m_groups;
  std::size_t m_curIndent;
  bool m_hasAnchor;
  bool m_hasAlias;
  bool m_hasTag;
  bool m_hasNonContent;
  std::size_t m_docCount;
};

template <typename T>
void EmitterState::_Set(Setting<T>& fmt, T value, FmtScope scope) {
  switch (scope) {
    case FmtScope::Local:
      m_modifiedSettings.push(fmt.set(value));
      break;
    case FmtScope::Global:
      fmt.set(value);
      m_globalModifiedSettings.push(
          fmt.set(value));  // this pushes an identity set, so when we restore,
      // it restores to the value here, and not the previous one
      break;
    default:
      assert(false);
  }
}
}  // namespace YAML

#endif  // EMITTERSTATE_H_62B23520_7C8E_11DE_8A39_0800200C9A66

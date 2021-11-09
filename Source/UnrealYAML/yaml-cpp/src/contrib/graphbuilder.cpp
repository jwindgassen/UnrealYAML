#include "contrib/graphbuilder.h"
#include "graphbuilderadapter.h"

#include "parser.h"

namespace YAML {
class GraphBuilderInterface;

void* BuildGraphOfNextDocument(Parser& parser,
                               GraphBuilderInterface& graphBuilder) {
  GraphBuilderAdapter eventHandler(graphBuilder);
  if (parser.HandleNextDocument(eventHandler)) {
    return eventHandler.RootNode();
  }
  return nullptr;
}
}  // namespace YAML

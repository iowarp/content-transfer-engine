@CLAUDE.md

We have added a new feature called compose to Chimaera. It requires changes to CreateParams.
The PoolConfig config_ parameter should be loaded using the existing configuration parsing system core_config.h.
Read @docs/chimaera/MODULE_DEVELOPMENT_GUIDE.md to see the new changes. Ensure that the
new code compiles. Prioritize getting things compiling.

We will remove the utility script launch_cte and instead use chimaera_compose. 

Document every parameter of the CTE configuration under @docs/config.md

Let's remove the ConfigurationManager GetInstance method. Instead, we should store the configuration directly in 
class ContentTransferEngine.

"""
Re-export `internalBinding` to JS
"""

import pythonmonkey as pm

"""
See function declarations in ./internal-binding.d.ts
"""
exports = pm.internalBinding  # type: ignore

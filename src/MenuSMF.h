#pragma once

namespace SimpleStagger
{
    /// SKSE Menu Framework (SMF) integration — soft dependency.
    /// Call Register() from SKSEPluginLoad. If SMF DLL is absent, this is a no-op.
    namespace MenuSMF
    {
        /// Register the SimpleStagger settings page with SMF.
        /// Safe to call even when SKSEMenuFramework.dll is not installed;
        /// logs one INFO line and returns in that case.
        void Register();
    }
}

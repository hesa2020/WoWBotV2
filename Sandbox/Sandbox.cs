﻿using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security;
using System.Security.Permissions;
using System.Threading;
using Agony.Sandbox.Shared;
using System.Runtime.InteropServices;

namespace Agony.Sandbox
{
    /// <summary>
    ///     Sandbox main file, contains the static bootstrap to instance a new Sandbox which acts as a safe Application Domain
    ///     (Manager).
    /// </summary>
    public static class Sandbox
    {
        /// <summary>
        ///     Static Constructor
        /// </summary>
        static Sandbox()
        {
            CLRByPasser.ByPass();
            AppDomain.CurrentDomain.ProcessExit += DomainOnProcessExit;
            AppDomain.CurrentDomain.AssemblyResolve += SandboxDomain.DomainOnAssemblyResolve;
            AppDomain.CurrentDomain.UnhandledException += delegate(object sender, UnhandledExceptionEventArgs args)
            {
                (new PermissionSet(PermissionState.Unrestricted)).Assert();
                Logs.Log("Sandbox: Unhandled Sandbox exception");
                #if DEBUG
                var securityException = args.ExceptionObject as SecurityException;
                if (securityException != null)
                {
                    Logs.Log(args.ExceptionObject.ToString());
                }
                #endif
                Logs.PrintException(args.ExceptionObject);
                CodeAccessPermission.RevertAssert();
            };
        }

        private static void DomainOnProcessExit(object sender, EventArgs e)
        {
            Logs.Log("Sandbox: Shutting down...");
        }

        /// <summary>
        ///     Current Process Identity (ID).
        /// </summary>
        internal static int Pid
        {
            get { return Process.GetCurrentProcess().Id; }
        }

        internal static bool AllAddonsLoaded { get; set; }

        /// <summary>
        ///     Bootstrap of the Sandbox (Secure Application Domain) from an external source.
        /// </summary>
        /// <param name="param">Extra Paramaters</param>
        /// <returns>
        ///     Bootstrap will return 0 if succeed to create a secure application domain on the machine and additional
        ///     features which will be needed to control the application domain without any problems. Bootstrap will return 1 if
        ///     any of the bootstrap actions will fail.
        /// </returns>

        //[RGiesecke.DllExport(CallingConvention = CallingConvention.Cdecl, ExportName = "Bootstrap")]
        public static int Bootstrap(string param)
        {
            try
            {
                Reload();
                Input.Subscribe();
            }
            catch (Exception e)
            {
                Logs.Log("Sandbox: Bootstrap error");
                Logs.Log(e.ToString());
                return 1;
            }

            return 0;
        }

        /// <summary>
        ///     Load Entry, will command the secure application domain to load in all of the assemblies which are passed in a list
        ///     from the loader.
        /// </summary>
        /// <returns>Whether the loading was successful</returns>
        internal static void Load()
        {
            // Set thread culture
            CultureInfo.DefaultThreadCurrentCulture = CultureInfo.InvariantCulture;
            CultureInfo.DefaultThreadCurrentUICulture = CultureInfo.InvariantCulture;
            Thread.CurrentThread.CurrentCulture = CultureInfo.InvariantCulture;
            Thread.CurrentThread.CurrentUICulture = CultureInfo.InvariantCulture;

            AllAddonsLoaded = false;
            if (SandboxDomain.Instance == null)
            {
                return;
            }
            try
            {
                //SandboxDomain.Instance.LoadAddon("D:\\AgonyWoW\\x64\\Debug\\Agony.Wrapper.dll", new string[1]);

                LoadLibrary("209d1d74adceabcd");
                LoadLibrary("a99070253df2afda");
                //SandboxDomain.Instance.LoadAddon("D:\\AgonyWoW\\x64\\Release\\Agony.SDK.dll", new string[1]);

                //SandboxDomain.Instance.LoadAddon("D:\\AgonyWoW\\x64\\Release\\Gathering.dll", new string[1]);

                //SandboxDomain.Instance.LoadAddon("D:\\AgonyWoW\\x64\\Release\\TestAddon.exe", new string[1]);

                var addons = ServiceFactory.CreateProxy<ILoaderService>().GetAssemblyList((int) 0);
                Logs.Log("Loading {0} Plugin{1}", addons.Count, addons.Count < 1 || addons.Count > 1 ? "s" : "");
                foreach (var assembly in addons)
                {
                    SandboxDomain.Instance.LoadAddon(assembly.PathToBinary, new string[1]);
                }
            }
            catch (Exception e)
            {
                Logs.Log("Sandbox: Loading assemblies failed");
                Logs.Log(e.ToString());
                return;
            }
            AllAddonsLoaded = true;
        }

        internal static void LoadLibrary(string publicToken)
        {
            foreach (var file in Directory.GetFiles(SandboxConfig.LibrariesDirectory))
            {
                try
                {
                    if (EqualsPublicToken(AssemblyName.GetAssemblyName(file), publicToken))
                    {
                        SandboxDomain.Instance.LoadAddon(file, new string[1]);
                        break;
                    }
                }
                catch (Exception)
                {
                    // ignored
                }
            }
        }

        internal static bool EqualsPublicToken(AssemblyName assemblyName, string publicToken)
        {
            return assemblyName.GetPublicKeyToken().Select(o => o.ToString("x2")).Concat(new[] { string.Empty }).Aggregate(string.Concat) == publicToken;
        }

        /// <summary>
        ///     Config Update Entry, will update the config from the loader to sync any changes.
        /// </summary>
        private static void UpdateConfig()
        {
            SandboxConfig.Reload();
        }

        /// <summary>
        ///     Reload Entry, will command the secure application domain to reload the given assemblies into the game.
        /// </summary>
        public static void Reload()
        {
            UpdateConfig();
            Unload();
            CreateApplicationDomain();
            Load();
        }

        /// <summary>
        ///     Recompile Entry, will command the secure application domain to request a recompile and afterwards to load the given
        ///     assemblies into the game.
        /// </summary>
        internal static void Recompile()
        {
            UpdateConfig();
            Unload();

            try
            {
                //ServiceFactory.CreateProxy<ILoaderService>().Recompile(Process.GetCurrentProcess().Id);
            }
            catch (Exception e)
            {
                Logs.Log("Sandbox: Remote recompiling failed");
                Logs.Log(e.ToString());
            }

            CreateApplicationDomain();
            Load();
        }

        /// <summary>
        ///     Unload Entry, will command the secure application domain to unload the current secure application domain.
        /// </summary>
        internal static void Unload()
        {
            AllAddonsLoaded = false;
            if (SandboxDomain.Instance == null)
            {
                return;
            }

            try
            {
                SandboxDomain.UnloadDomain(SandboxDomain.Instance);
            }
            catch (Exception e)
            {
                Logs.Log("Sandbox: Unloading AppDomain failed");
                //Logs.PrintException(e);
                Logs.Log(e.ToString());
            }
            SandboxDomain.Instance = null;
            Thread.Sleep(1000 * 2);
        }

        /// <summary>
        ///     Creation of a Secure Application Domain.
        /// </summary>
        private static void CreateApplicationDomain()
        {
            if (SandboxDomain.Instance != null)
            {
                return;
            }

            try
            {
                SandboxDomain.Instance = SandboxDomain.CreateDomain("SandboxDomain");

                if (SandboxDomain.Instance == null)
                {
                    Logs.Log("Sandbox: AppDomain creation failed, please report this error!");
                }
            }
            catch (Exception e)
            {
                Logs.Log("Sandbox: Error during AppDomain creation");
                Logs.Log(e.ToString());
            }
        }
    }
}

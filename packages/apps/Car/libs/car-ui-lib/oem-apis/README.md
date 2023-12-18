# Car-ui-lib OEM APIs

```
#############################################
#                  WARNING                  #
#############################################
# The OEM APIs as they appear on this       #
# branch of android are not finalized!      #
# If a shared library is built using them,  #
# it will cause apps to crash!              #
#                                           #
# Please get the OEM APIs from a later      #
# branch of android instead.                #
#############################################
```

These APIs allow OEMs to build a shared library for
car-ui-lib that can supply custom implementations
of car-ui-lib components. See
SharedLibraryFactorySingleton for information
on the entrypoint to the shared library.

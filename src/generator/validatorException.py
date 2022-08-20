"""This module defines user defined exceptions that will be used for validation."""


class EnumException(Exception):
    """User defined exception for faulty enum definitions."""

    pass


class GroupException(Exception):
    """User defined exception for faulty group definitions."""

    pass


class StructException(Exception):
    """User defined exception for faulty struct definitions."""

    pass


class DatapointException(Exception):
    """User defined exception for faulty data points definitions."""

    pass

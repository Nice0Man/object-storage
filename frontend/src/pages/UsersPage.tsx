import React, { useEffect, useState } from 'react';
import {
  Box,
  Typography,
  Button,
  Table,
  TableBody,
  TableCell,
  TableContainer,
  TableHead,
  TableRow,
  Paper,
  IconButton,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  TextField,
  Chip,
  FormControl,
  InputLabel,
  Select,
  MenuItem,
  OutlinedInput,
  SelectChangeEvent,
} from '@mui/material';
import { Add, Delete, Refresh, PersonAdd, Policy as PolicyIcon, Group as GroupIcon } from '@mui/icons-material';
import { useAppDispatch } from '../hooks/useAppDispatch';
import { useAppSelector } from '../hooks/useAppSelector';
import {
  fetchUsers,
  createUser,
  deleteUser,
  selectUsers,
  selectUsersLoading,
  selectUsersError,
  clearError,
} from '../store/usersSlice';
import Loader from '../components/Common/Loader';
import ErrorAlert from '../components/Common/ErrorAlert';
import UserPoliciesDialog from '../components/Users/UserPoliciesDialog';
import UserGroupsDialog from '../components/Users/UserGroupsDialog';
import { selectCan } from '../store/authSlice';

const UsersPage: React.FC = () => {
  const dispatch = useAppDispatch();
  const users = useAppSelector(selectUsers);
  const loading = useAppSelector(selectUsersLoading);
  const error = useAppSelector(selectUsersError);
  const canManageUsers = useAppSelector(selectCan("users.manage"));

  const [createDialogOpen, setCreateDialogOpen] = useState(false);
  const [deleteDialogOpen, setDeleteDialogOpen] = useState(false);
  const [selectedUser, setSelectedUser] = useState<string | null>(null);
  const [policiesDialogOpen, setPoliciesDialogOpen] = useState(false);
  const [groupsDialogOpen, setGroupsDialogOpen] = useState(false);
  const [selectedUserForManagement, setSelectedUserForManagement] = useState<{
    accessKey: string;
    username: string;
    groups?: string[];
  } | null>(null);

  // Form fields
  const [accessKey, setAccessKey] = useState('');
  const [secretKey, setSecretKey] = useState('');
  const [selectedPolicies, setSelectedPolicies] = useState<string[]>([]);
  const [selectedGroups, setSelectedGroups] = useState<string[]>([]);

  // Predefined policies and groups (these should come from API in production)
  const availablePolicies = ['readwrite', 'readonly', 'writeonly', 'admin'];
  const availableGroups = ['developers', 'admins', 'viewers'];

  useEffect(() => {
    dispatch(fetchUsers());
  }, [dispatch]);

  const handleRefresh = () => {
    dispatch(fetchUsers());
  };

  const handleCreateUser = async () => {
    if (!accessKey || !secretKey) return;

    try {
      await dispatch(
        createUser({
          access_key: accessKey,
          secret_key: secretKey,
          policies: selectedPolicies,
          groups: selectedGroups,
        })
      ).unwrap();
      resetForm();
      setCreateDialogOpen(false);
    } catch (err) {
      // Error is handled by the slice
    }
  };

  const handleDeleteUser = async () => {
    if (!selectedUser) return;

    try {
      await dispatch(deleteUser(selectedUser)).unwrap();
      setDeleteDialogOpen(false);
      setSelectedUser(null);
    } catch (err) {
      // Error is handled by the slice
    }
  };

  const openDeleteDialog = (accessKey: string) => {
    setSelectedUser(accessKey);
    setDeleteDialogOpen(true);
  };

  const openPoliciesDialog = (accessKey: string, username: string) => {
    setSelectedUserForManagement({ accessKey, username });
    setPoliciesDialogOpen(true);
  };

  const openGroupsDialog = (accessKey: string, username: string, groups?: string[]) => {
    setSelectedUserForManagement({ accessKey, username, groups });
    setGroupsDialogOpen(true);
  };

  const resetForm = () => {
    setAccessKey('');
    setSecretKey('');
    setSelectedPolicies([]);
    setSelectedGroups([]);
  };

  const handlePoliciesChange = (event: SelectChangeEvent<typeof selectedPolicies>) => {
    const value = event.target.value;
    setSelectedPolicies(typeof value === 'string' ? value.split(',') : value);
  };

  const handleGroupsChange = (event: SelectChangeEvent<typeof selectedGroups>) => {
    const value = event.target.value;
    setSelectedGroups(typeof value === 'string' ? value.split(',') : value);
  };

  const formatDate = (dateString: string) => {
    return new Date(dateString).toLocaleString();
  };

  if (loading && users.length === 0) {
    return <Loader message="Loading users..." />;
  }

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%', minHeight: '70vh' }}>
      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 3 }}>
        <Typography variant="h4">Users</Typography>
        <Box>
          <IconButton onClick={handleRefresh} sx={{ mr: 1 }}>
            <Refresh />
          </IconButton>
          <Button
            variant="contained"
            startIcon={<Add />}
            onClick={() => setCreateDialogOpen(true)}
            disabled={!canManageUsers}
          >
            Create User
          </Button>
        </Box>
      </Box>

      <ErrorAlert error={error} onClose={() => dispatch(clearError())} />

      {users.length === 0 ? (
        <Box sx={{ textAlign: 'center', py: 8 }}>
          <PersonAdd sx={{ fontSize: 100, color: 'text.secondary', mb: 2 }} />
          <Typography variant="h6" color="text.secondary" gutterBottom>
            No users found
          </Typography>
          <Typography variant="body2" color="text.secondary" paragraph>
            Create your first user to start managing access
          </Typography>
          <Button
            variant="contained"
            startIcon={<Add />}
            onClick={() => setCreateDialogOpen(true)}
            disabled={!canManageUsers}
          >
            Create User
          </Button>
        </Box>
      ) : (
        <TableContainer component={Paper} sx={{ flexGrow: 1 }}>
          <Table>
            <TableHead>
              <TableRow>
                <TableCell>Access Key</TableCell>
                <TableCell>Status</TableCell>
                <TableCell>Policies</TableCell>
                <TableCell>Groups</TableCell>
                <TableCell>Created</TableCell>
                <TableCell align="right">Actions</TableCell>
              </TableRow>
            </TableHead>
            <TableBody>
              {users.map((user) => (
                <TableRow key={user.access_key} hover>
                  <TableCell>
                    <Typography variant="body2" fontWeight="medium">
                      {user.access_key}
                    </Typography>
                  </TableCell>
                  <TableCell>
                    <Chip
                      label={user.status}
                      size="small"
                      color={user.status === 'enabled' ? 'success' : 'default'}
                    />
                  </TableCell>
                  <TableCell>
                    <Box sx={{ display: 'flex', gap: 0.5, flexWrap: 'wrap' }}>
                      {user.policies && user.policies.length > 0 ? (
                        user.policies.map((policy) => (
                          <Chip key={policy} label={policy} size="small" variant="outlined" />
                        ))
                      ) : (
                        <Typography variant="caption" color="text.secondary">
                          No policies
                        </Typography>
                      )}
                    </Box>
                  </TableCell>
                  <TableCell>
                    <Box sx={{ display: 'flex', gap: 0.5, flexWrap: 'wrap' }}>
                      {user.groups && user.groups.length > 0 ? (
                        user.groups.map((group) => (
                          <Chip key={group} label={group} size="small" variant="outlined" color="primary" />
                        ))
                      ) : (
                        <Typography variant="caption" color="text.secondary">
                          No groups
                        </Typography>
                      )}
                    </Box>
                  </TableCell>
                  <TableCell>
                    <Typography variant="caption">{formatDate(user.created_at)}</Typography>
                  </TableCell>
                  <TableCell align="right">
                    <IconButton
                      size="small"
                      color="primary"
                      onClick={() => openPoliciesDialog(user.access_key, user.access_key)}
                      title="Manage Policies"
                      disabled={!canManageUsers}
                    >
                      <PolicyIcon />
                    </IconButton>
                    <IconButton
                      size="small"
                      color="info"
                      onClick={() => openGroupsDialog(user.access_key, user.access_key, user.groups)}
                      title="Manage Groups"
                      disabled={!canManageUsers}
                    >
                      <GroupIcon />
                    </IconButton>
                    <IconButton
                      size="small"
                      color="error"
                      onClick={() => openDeleteDialog(user.access_key)}
                      title="Delete User"
                      disabled={!canManageUsers}
                    >
                      <Delete />
                    </IconButton>
                  </TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </TableContainer>
      )}

      {/* Create User Dialog */}
      <Dialog open={createDialogOpen} onClose={() => setCreateDialogOpen(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Create New User</DialogTitle>
        <DialogContent>
            <TextField
              margin="dense"
              label="Access Key"
              fullWidth
              value={accessKey}
              onChange={(e) => setAccessKey(e.target.value)}
              helperText="Unique identifier for the user (e.g., username)"
              sx={{ mb: 2 }}
            />
          <TextField
            margin="dense"
            label="Secret Key"
            type="password"
            fullWidth
            value={secretKey}
            onChange={(e) => setSecretKey(e.target.value)}
            helperText="Password for authentication (keep it secure)"
            sx={{ mb: 2 }}
          />

          <FormControl fullWidth sx={{ mb: 2 }}>
            <InputLabel>Policies</InputLabel>
            <Select
              multiple
              value={selectedPolicies}
              onChange={handlePoliciesChange}
              input={<OutlinedInput label="Policies" />}
              renderValue={(selected) => (
                <Box sx={{ display: 'flex', flexWrap: 'wrap', gap: 0.5 }}>
                  {selected.map((value) => (
                    <Chip key={value} label={value} size="small" />
                  ))}
                </Box>
              )}
            >
              {availablePolicies.map((policy) => (
                <MenuItem key={policy} value={policy}>
                  {policy}
                </MenuItem>
              ))}
            </Select>
          </FormControl>

          <FormControl fullWidth>
            <InputLabel>Groups</InputLabel>
            <Select
              multiple
              value={selectedGroups}
              onChange={handleGroupsChange}
              input={<OutlinedInput label="Groups" />}
              renderValue={(selected) => (
                <Box sx={{ display: 'flex', flexWrap: 'wrap', gap: 0.5 }}>
                  {selected.map((value) => (
                    <Chip key={value} label={value} size="small" color="primary" />
                  ))}
                </Box>
              )}
            >
              {availableGroups.map((group) => (
                <MenuItem key={group} value={group}>
                  {group}
                </MenuItem>
              ))}
            </Select>
          </FormControl>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setCreateDialogOpen(false)}>Cancel</Button>
          <Button
            onClick={handleCreateUser}
            variant="contained"
            disabled={!accessKey || !secretKey || loading || !canManageUsers}
          >
            Create
          </Button>
        </DialogActions>
      </Dialog>

      {/* Delete User Dialog */}
      <Dialog open={deleteDialogOpen} onClose={() => setDeleteDialogOpen(false)}>
        <DialogTitle>Delete User</DialogTitle>
        <DialogContent>
          <Typography>
            Are you sure you want to delete user <strong>{selectedUser}</strong>?
          </Typography>
          <Typography color="error" sx={{ mt: 2 }}>
            This action cannot be undone. The user will lose access immediately.
          </Typography>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setDeleteDialogOpen(false)}>Cancel</Button>
          <Button onClick={handleDeleteUser} color="error" variant="contained" disabled={loading}>
            Delete
          </Button>
        </DialogActions>
      </Dialog>

      {/* User Policies Dialog */}
      {selectedUserForManagement && (
        <UserPoliciesDialog
          open={policiesDialogOpen}
          onClose={() => {
            setPoliciesDialogOpen(false);
            setSelectedUserForManagement(null);
            handleRefresh();
          }}
          accessKey={selectedUserForManagement.accessKey}
          username={selectedUserForManagement.username}
        />
      )}

      {/* User Groups Dialog */}
      {selectedUserForManagement && (
        <UserGroupsDialog
          open={groupsDialogOpen}
          onClose={() => {
            setGroupsDialogOpen(false);
            setSelectedUserForManagement(null);
          }}
          accessKey={selectedUserForManagement.accessKey}
          username={selectedUserForManagement.username}
          groups={selectedUserForManagement.groups}
          onUpdate={handleRefresh}
        />
      )}
    </Box>
  );
};

export default UsersPage;

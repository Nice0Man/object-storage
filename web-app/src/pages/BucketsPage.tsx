import React, { useEffect, useState } from 'react';
import {
  Box,
  Typography,
  Button,
  Card,
  CardContent,
  CardActions,
  Grid,
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  TextField,
  IconButton,
  Chip,
  FormControlLabel,
  Switch,
} from '@mui/material';
import {
  Add,
  Delete,
  Folder,
  Refresh,
  Policy as PolicyIcon,
} from '@mui/icons-material';
import { useNavigate } from 'react-router-dom';
import { useAppDispatch } from '../hooks/useAppDispatch';
import { useAppSelector } from '../hooks/useAppSelector';
import {
  fetchBuckets,
  createBucket,
  deleteBucket,
  selectBuckets,
  selectBucketsLoading,
  selectBucketsError,
  clearError,
} from '../store/bucketsSlice';
import Loader from '../components/Common/Loader';
import ErrorAlert from '../components/Common/ErrorAlert';
import BucketPolicyDialog from '../components/Buckets/BucketPolicyDialog';

const BucketsPage: React.FC = () => {
  const navigate = useNavigate();
  const dispatch = useAppDispatch();
  const buckets = useAppSelector(selectBuckets);
  const loading = useAppSelector(selectBucketsLoading);
  const error = useAppSelector(selectBucketsError);

  const [createDialogOpen, setCreateDialogOpen] = useState(false);
  const [deleteDialogOpen, setDeleteDialogOpen] = useState(false);
  const [policyDialogOpen, setPolicyDialogOpen] = useState(false);
  const [selectedBucket, setSelectedBucket] = useState<string | null>(null);
  const [newBucketName, setNewBucketName] = useState('');
  const [versioning, setVersioning] = useState(false);
  const [objectLocking, setObjectLocking] = useState(false);

  useEffect(() => {
    dispatch(fetchBuckets());
  }, [dispatch]);

  const handleRefresh = () => {
    dispatch(fetchBuckets());
  };

  const handleCreateBucket = async () => {
    if (!newBucketName) return;

    try {
      await dispatch(
        createBucket({
          name: newBucketName,
          versioning,
          object_locking: objectLocking,
        })
      ).unwrap();
      setCreateDialogOpen(false);
      setNewBucketName('');
      setVersioning(false);
      setObjectLocking(false);
    } catch (err) {
      // Error is handled by the slice
    }
  };

  const handleDeleteBucket = async () => {
    if (!selectedBucket) return;

    try {
      await dispatch(deleteBucket(selectedBucket)).unwrap();
      setDeleteDialogOpen(false);
      setSelectedBucket(null);
    } catch (err) {
      // Error is handled by the slice
    }
  };

  const openDeleteDialog = (bucketName: string) => {
    setSelectedBucket(bucketName);
    setDeleteDialogOpen(true);
  };

  const openPolicyDialog = (bucketName: string) => {
    setSelectedBucket(bucketName);
    setPolicyDialogOpen(true);
  };

  const formatDate = (dateString: string) => {
    return new Date(dateString).toLocaleString();
  };

  const formatSize = (bytes: number) => {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i];
  };

  if (loading && buckets.length === 0) {
    return <Loader message="Loading buckets..." />;
  }

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', height: '100%', minHeight: '70vh' }}>
      <Box sx={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', mb: 3 }}>
        <Typography variant="h4">Buckets</Typography>
        <Box>
          <IconButton onClick={handleRefresh} sx={{ mr: 1 }}>
            <Refresh />
          </IconButton>
          <Button
            variant="contained"
            startIcon={<Add />}
            onClick={() => setCreateDialogOpen(true)}
          >
            Create Bucket
          </Button>
        </Box>
      </Box>

      <ErrorAlert error={error} onClose={() => dispatch(clearError())} />

      {buckets.length === 0 ? (
        <Box sx={{ textAlign: 'center', py: 8 }}>
          <Folder sx={{ fontSize: 100, color: 'text.secondary', mb: 2 }} />
          <Typography variant="h6" color="text.secondary" gutterBottom>
            No buckets found
          </Typography>
          <Typography variant="body2" color="text.secondary" paragraph>
            Create your first bucket to start storing objects
          </Typography>
          <Button
            variant="contained"
            startIcon={<Add />}
            onClick={() => setCreateDialogOpen(true)}
          >
            Create Bucket
          </Button>
        </Box>
      ) : (
        <Grid container spacing={3}>
          {buckets.map((bucket) => (
            <Grid item xs={12} sm={6} md={4} key={bucket.name}>
              <Card sx={{ height: '100%', display: 'flex', flexDirection: 'column' }}>
                <CardContent sx={{ flexGrow: 1 }}>
                  <Box sx={{ display: 'flex', alignItems: 'center', mb: 2 }}>
                    <Folder sx={{ fontSize: 40, color: 'primary.main', mr: 2 }} />
                    <Typography variant="h6" component="div">
                      {bucket.name}
                    </Typography>
                  </Box>
                  <Box sx={{ mb: 1 }}>
                    <Chip
                      label={`${bucket.objects_count || 0} objects`}
                      size="small"
                      sx={{ mr: 1 }}
                    />
                    <Chip
                      label={formatSize(bucket.size || 0)}
                      size="small"
                      color="primary"
                    />
                  </Box>
                  <Typography variant="caption" color="text.secondary" display="block">
                    Created: {formatDate(bucket.creation_date)}
                  </Typography>
                </CardContent>
                <CardActions>
                  <Button
                    size="small"
                    startIcon={<Folder />}
                    onClick={() => navigate(`/objects?bucket=${bucket.name}`)}
                  >
                    Browse
                  </Button>
                  <IconButton
                    size="small"
                    color="primary"
                    onClick={() => openPolicyDialog(bucket.name)}
                    title="Manage Policy"
                  >
                    <PolicyIcon />
                  </IconButton>
                  <IconButton
                    size="small"
                    color="error"
                    onClick={() => openDeleteDialog(bucket.name)}
                    sx={{ ml: 'auto' }}
                  >
                    <Delete />
                  </IconButton>
                </CardActions>
              </Card>
            </Grid>
          ))}
        </Grid>
      )}

      {/* Create Bucket Dialog */}
      <Dialog open={createDialogOpen} onClose={() => setCreateDialogOpen(false)} maxWidth="sm" fullWidth>
        <DialogTitle>Create New Bucket</DialogTitle>
        <DialogContent>
          <TextField
            autoFocus
            margin="dense"
            label="Bucket Name"
            fullWidth
            value={newBucketName}
            onChange={(e) => setNewBucketName(e.target.value)}
            helperText="Bucket names must be unique and follow DNS naming conventions"
          />
          <FormControlLabel
            control={
              <Switch
                checked={versioning}
                onChange={(e) => setVersioning(e.target.checked)}
              />
            }
            label="Enable Versioning"
            sx={{ mt: 2, display: 'block' }}
          />
          <FormControlLabel
            control={
              <Switch
                checked={objectLocking}
                onChange={(e) => setObjectLocking(e.target.checked)}
              />
            }
            label="Enable Object Locking"
            sx={{ display: 'block' }}
          />
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setCreateDialogOpen(false)}>Cancel</Button>
          <Button
            onClick={handleCreateBucket}
            variant="contained"
            disabled={!newBucketName || loading}
          >
            Create
          </Button>
        </DialogActions>
      </Dialog>

      {/* Delete Bucket Dialog */}
      <Dialog open={deleteDialogOpen} onClose={() => setDeleteDialogOpen(false)}>
        <DialogTitle>Delete Bucket</DialogTitle>
        <DialogContent>
          <Typography>
            Are you sure you want to delete bucket <strong>{selectedBucket}</strong>?
          </Typography>
          <Typography color="error" sx={{ mt: 2 }}>
            This action cannot be undone. All objects in the bucket will be lost.
          </Typography>
        </DialogContent>
        <DialogActions>
          <Button onClick={() => setDeleteDialogOpen(false)}>Cancel</Button>
          <Button onClick={handleDeleteBucket} color="error" variant="contained" disabled={loading}>
            Delete
          </Button>
        </DialogActions>
      </Dialog>

      {/* Bucket Policy Dialog */}
      {selectedBucket && (
        <BucketPolicyDialog
          open={policyDialogOpen}
          onClose={() => {
            setPolicyDialogOpen(false);
            setSelectedBucket(null);
          }}
          bucketName={selectedBucket}
        />
      )}
    </Box>
  );
};

export default BucketsPage;


import React from "react";
import {
  IconButton,
  Menu,
  MenuItem,
  ListItemIcon,
  ListItemText,
  Divider,
} from "@mui/material";
import {
  MoreVert,
  Info,
  Download,
  FileCopy,
  Label,
  Link,
  Delete,
  History,
  Lock,
  Visibility,
} from "@mui/icons-material";
import { useTranslation } from "react-i18next";

interface ObjectActionsMenuProps {
  onPreview?: () => void;
  onInfo: () => void;
  onDownload: () => void;
  onCopy: () => void;
  onTags: () => void;
  onPresignedUrl: () => void;
  onDelete: () => void;
  onVersions?: () => void;
  onRetention?: () => void;
  readOnly?: boolean;
}

const ObjectActionsMenu: React.FC<ObjectActionsMenuProps> = ({
  onPreview,
  onInfo,
  onDownload,
  onCopy,
  onTags,
  onPresignedUrl,
  onDelete,
  onVersions,
  onRetention,
  readOnly = false,
}) => {
  const { t } = useTranslation();
  const [anchorEl, setAnchorEl] = React.useState<null | HTMLElement>(null);
  const open = Boolean(anchorEl);

  const handleClick = (event: React.MouseEvent<HTMLElement>) => {
    event.stopPropagation();
    setAnchorEl(event.currentTarget);
  };

  const handleClose = () => {
    setAnchorEl(null);
  };

  const handleAction = (action: () => void) => {
    action();
    handleClose();
  };

  return (
    <>
      <IconButton size="small" onClick={handleClick} aria-label="more actions" disabled={readOnly}>
        <MoreVert />
      </IconButton>
      <Menu
        anchorEl={anchorEl}
        open={open}
        onClose={handleClose}
        onClick={(e) => e.stopPropagation()}
        transformOrigin={{ horizontal: "right", vertical: "top" }}
        anchorOrigin={{ horizontal: "right", vertical: "bottom" }}
      >
        {onPreview && (
          <MenuItem onClick={() => handleAction(onPreview)}>
            <ListItemIcon>
              <Visibility fontSize="small" />
            </ListItemIcon>
            <ListItemText>Preview</ListItemText>
          </MenuItem>
        )}
        <MenuItem onClick={() => handleAction(onInfo)}>
          <ListItemIcon>
            <Info fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.info")}</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => handleAction(onDownload)}>
          <ListItemIcon>
            <Download fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.download")}</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => handleAction(onCopy)} disabled={readOnly}>
          <ListItemIcon>
            <FileCopy fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.copy")}</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => handleAction(onTags)} disabled={readOnly}>
          <ListItemIcon>
            <Label fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.tags")}</ListItemText>
        </MenuItem>
        <MenuItem onClick={() => handleAction(onPresignedUrl)} disabled={readOnly}>
          <ListItemIcon>
            <Link fontSize="small" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.generateLink")}</ListItemText>
        </MenuItem>
        {onVersions && (
          <MenuItem onClick={() => handleAction(onVersions)}>
            <ListItemIcon>
              <History fontSize="small" />
            </ListItemIcon>
            <ListItemText>Versions</ListItemText>
          </MenuItem>
        )}
        {onRetention && (
          <MenuItem onClick={() => handleAction(onRetention)} disabled={readOnly}>
            <ListItemIcon>
              <Lock fontSize="small" />
            </ListItemIcon>
            <ListItemText>Retention & Lock</ListItemText>
          </MenuItem>
        )}
        <Divider />
        <MenuItem
          onClick={() => handleAction(onDelete)}
          disabled={readOnly}
          sx={{ color: "error.main" }}
        >
          <ListItemIcon>
            <Delete fontSize="small" color="error" />
          </ListItemIcon>
          <ListItemText>{t("objects.actions.delete")}</ListItemText>
        </MenuItem>
      </Menu>
    </>
  );
};

export default ObjectActionsMenu;
